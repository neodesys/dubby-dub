/**
 * dubby-dub
 *
 * Copyright (C) 2020, Loïc Le Page
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */
#include "../exceptions.h"
#include "IPlayerListener.h"
#include <cassert>

const GQuark Player::errorDomain = Glib::Quark("PlayerErrorDomain");

Player::Player()
    : m_busWatchId(0), m_prerollingPads(1), m_prerollDone(false), m_currentState(State::stopped),
      m_pendingState(State::undefined), m_interrupted(false)
{
    m_pipeline = Gst::Pipeline::create();
    m_uriDecodeBin = Gst::UriDecodeBin::create();

    try
    {
        m_busWatchId = m_pipeline->get_bus()->add_watch(sigc::mem_fun(*this, &Player::onBusMessage));
        m_pipeline->add(m_uriDecodeBin);

        m_uriDecodeBin->signal_pad_added().connect(sigc::mem_fun(*this, &Player::onPadAdded));
        m_uriDecodeBin->signal_no_more_pads().connect([this]() {
            // WARNING: called from any streaming thread.
            this->onPadPrerolled();
        });
    }
    catch (const std::exception& e)
    {
        throw UnrecoverableError();
    }
}

Player::~Player()
{
    if (hasStableState(State::playing))
    {
        m_interrupted = true;
        m_pendingState = State::stopped;
    }
    stop();
    m_pipeline->get_bus()->remove_watch(m_busWatchId);
}

void Player::addPlayerListener(const std::shared_ptr<IPlayerListener>& listener) noexcept
{
    if (listener)
    {
        for (auto it = m_listeners.begin(); it != m_listeners.end();)
        {
            auto entry = it->lock();
            if (entry)
            {
                if (entry == listener)
                {
                    return;
                }
                ++it;
            }
            else
            {
                it = m_listeners.erase(it);
            }
        }

        m_listeners.push_back(listener);
    }
}

void Player::removePlayerListener(const std::shared_ptr<IPlayerListener>& listener) noexcept
{
    if (listener)
    {
        for (auto it = m_listeners.begin(); it != m_listeners.end();)
        {
            auto entry = it->lock();
            if (entry)
            {
                if (entry == listener)
                {
                    m_listeners.erase(it);
                    return;
                }
                ++it;
            }
            else
            {
                it = m_listeners.erase(it);
            }
        }
    }
}

void Player::forEachConnector(const std::function<void(Connector&)>& cb)
{
    if (!hasStableState(State::prerolled))
    {
        throw InvalidStateException();
    }

    for (auto& connector : m_connectors)
    {
        cb(connector);
    }
}

bool Player::hasStableState(State state) const noexcept
{
    return (m_currentState == state) && (m_pendingState == State::undefined);
}

void Player::play(const Glib::ustring& uri)
{
    if (!hasStableState(State::stopped))
    {
        throw InvalidStateException();
    }

    assert(m_connectors.empty()); // NOLINT

    m_prerollingPads = 1;
    m_prerollDone = false;
    m_pendingState = State::prerolled;
    m_interrupted = false;
    m_uriDecodeBin->property_uri() = uri;
    m_pipeline->set_state(Gst::STATE_PAUSED);
}

void Player::stop() noexcept
{
    if (hasStableState(State::playing))
    {
        m_interrupted = true;
        m_pipeline->send_event(Gst::EventEos::create());
    }
    else if (!hasStableState(State::stopped))
    {
        m_pipeline->set_state(Gst::STATE_NULL);

        m_currentState = State::stopped;
        m_pendingState = State::undefined;
        triggerPlayerStopped();

        // Stop is always called from main thread but it can be called
        // BEFORE the "preroll_done" message (in particular in case of error).
        // So we must protect write access to the m_connectors vector as we
        // may still have concurrent accesses from onPadAdded probes.
        const std::lock_guard<std::mutex> lock(m_connectorsWriteLock);
        m_prerollDone = true;
        m_connectors.clear();
    }
}

bool Player::onBusMessage(const Glib::RefPtr<Gst::Bus>& bus, const Glib::RefPtr<Gst::Message>& message) noexcept
{
    switch (message->get_message_type())
    {
    case Gst::MESSAGE_STATE_CHANGED: {
        auto msgStateChanged = Glib::RefPtr<Gst::MessageStateChanged>::cast_static(message);
        if ((msgStateChanged->parse_new_state() == Gst::STATE_PLAYING) && (m_pendingState == State::playing))
        {
            m_currentState = State::playing;
            m_pendingState = State::undefined;
            triggerPlayerPlaying();
        }
        break;
    }

    case Gst::MESSAGE_APPLICATION: {
        auto msgApplication = Glib::RefPtr<Gst::MessageApplication>::cast_static(message);
        Glib::ustring name = msgApplication->get_structure().get_name();
        if ((name == "preroll_done") && (m_pendingState == State::prerolled))
        {
            m_prerollDone = true;
            m_currentState = State::prerolled;
            m_pendingState = State::undefined;
            try
            {
                triggerPlayerPrerolled();

                for (auto& connector : m_connectors)
                {
                    connector.unblock();
                }

                m_pendingState = State::playing;
                m_pipeline->set_state(Gst::STATE_PLAYING);
            }
            catch (const std::exception& e)
            {
                bus->post(Gst::MessageError::create(m_pipeline,
                                                    Glib::Error(errorDomain,
                                                                static_cast<int>(ErrorCode::cannotInitializePipeline),
                                                                "cannot initialize pipeline"),
                                                    e.what()));
            }
        }
        break;
    }

    case Gst::MESSAGE_EOS:
        m_pendingState = State::stopped;
        stop();
        break;

    case Gst::MESSAGE_ERROR: {
        Glib::Error error;
        std::string debugMessage;
        auto msgError = Glib::RefPtr<Gst::MessageError>::cast_static(message);
        if (msgError)
        {
            error = msgError->parse_error();
            debugMessage = msgError->parse_debug();
        }
        else
        {
            error = Glib::Error(errorDomain, static_cast<int>(ErrorCode::undefined), "undefined error");
        }
        stop();
        triggerPipelineIssue(true, error, debugMessage);
        break;
    }

    case Gst::MESSAGE_WARNING: {
        Glib::Error error;
        std::string debugMessage;
        auto msgWarning = Glib::RefPtr<Gst::MessageWarning>::cast_static(message);
        if (msgWarning)
        {
            error = msgWarning->parse_error();
            debugMessage = msgWarning->parse_debug();
        }
        else
        {
            error = Glib::Error(errorDomain, static_cast<int>(ErrorCode::undefined), "undefined error");
        }
        triggerPipelineIssue(false, error, debugMessage);
        break;
    }

    default:
        break;
    }

    return true;
}

void Player::onPadAdded(const Glib::RefPtr<Gst::Pad>& pad) noexcept
{
    // WARNING: called from any streaming thread.
    if (m_prerollDone)
    {
        return;
    }

    ++this->m_prerollingPads;

    try
    {
        Connector connector(pad, [this](const Glib::RefPtr<Gst::Pad>& /*pad*/, const Gst::PadProbeInfo& /*info*/) {
            // WARNING: called from any streaming thread.
            this->onPadPrerolled();
            return Gst::PAD_PROBE_OK;
        });

        // This method may be called from any streaming thread depending on the pad added,
        // so we need to protect m_connectors against race conditions. After "preroll_done"
        // message has been received on main thread (with a memory barrier), m_connectors
        // vector is exclusively used from the main thread and thus doesn't need special
        // synchronization anymore.
        const std::lock_guard<std::mutex> lock(m_connectorsWriteLock);
        if (!m_prerollDone)
        {
            m_connectors.push_back(std::move(connector));
        }
    }
    catch (const std::exception& e)
    {
        m_pipeline->get_bus()->post(
            Gst::MessageError::create(m_uriDecodeBin,
                                      Glib::Error(errorDomain, static_cast<int>(ErrorCode::cannotCreateConnector),
                                                  "cannot create output connector"),
                                      e.what()));
    }
}

void Player::onPadPrerolled() noexcept
{
    // WARNING: called from any streaming thread.
    if (m_prerollDone)
    {
        return;
    }

    if (--m_prerollingPads <= 0)
    {
        m_pipeline->get_bus()->post(Gst::MessageApplication::create(m_uriDecodeBin, Gst::Structure("preroll_done")));
    }
}

void Player::triggerPlayerPrerolled()
{
    for (auto it = m_listeners.begin(); it != m_listeners.end();)
    {
        auto listener = it->lock();
        if (listener)
        {
            listener->onPlayerPrerolled(*this);
            ++it;
        }
        else
        {
            it = m_listeners.erase(it);
        }
    }
}

void Player::triggerPlayerPlaying() noexcept
{
    for (auto it = m_listeners.begin(); it != m_listeners.end();)
    {
        auto listener = it->lock();
        if (listener)
        {
            listener->onPlayerPlaying(*this);
            ++it;
        }
        else
        {
            it = m_listeners.erase(it);
        }
    }
}

void Player::triggerPlayerStopped() noexcept
{
    for (auto it = m_listeners.begin(); it != m_listeners.end();)
    {
        auto listener = it->lock();
        if (listener)
        {
            listener->onPlayerStopped(*this, m_interrupted);
            ++it;
        }
        else
        {
            it = m_listeners.erase(it);
        }
    }
}

void Player::triggerPipelineIssue(bool isFatalError, const Glib::Error& error, const std::string& debugMessage) noexcept
{
    for (auto it = m_listeners.begin(); it != m_listeners.end();)
    {
        auto listener = it->lock();
        if (listener)
        {
            listener->onPipelineIssue(*this, isFatalError, error, debugMessage);
            ++it;
        }
        else
        {
            it = m_listeners.erase(it);
        }
    }
}
