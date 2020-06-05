#include "Transcoder.h"
#include "exceptions.h"
#include <cassert>

const GQuark Transcoder::errorDomain = Glib::Quark("TranscoderErrorDomain");

Transcoder::Transcoder()
    : m_prerollingPads(1), m_prerollDone(false), m_currentState(State::stopped), m_pendingState(State::undefined),
      m_interrupted(false)
{
    m_pipeline = Gst::Pipeline::create();
    m_uriDecodeBin = Gst::UriDecodeBin::create();

    try
    {
        m_busWatchId = m_pipeline->get_bus()->add_watch(sigc::mem_fun(*this, &Transcoder::onBusMessage));
        m_pipeline->add(m_uriDecodeBin);

        m_uriDecodeBin->signal_pad_added().connect(sigc::mem_fun(*this, &Transcoder::onPadAdded));
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

Transcoder::~Transcoder()
{
    if (hasStableState(State::transcoding))
    {
        m_interrupted = true;
        m_pendingState = State::stopped;
    }
    stopTranscoding();
    m_pipeline->get_bus()->remove_watch(m_busWatchId);
}

void Transcoder::addEncoder(const std::shared_ptr<Encoder>& encoder)
{
    if (!hasStableState(State::stopped))
    {
        throw InvalidStateException();
    }

    if (encoder && (std::find(m_encoders.begin(), m_encoders.end(), encoder) == m_encoders.end()))
    {
        m_encoders.push_back(encoder);
    }
}

void Transcoder::clearEncoders()
{
    if (!hasStableState(State::stopped))
    {
        throw InvalidStateException();
    }

    m_encoders.clear();
}

void Transcoder::addTranscoderListener(const std::shared_ptr<ITranscoderListener>& listener) noexcept
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

void Transcoder::removeTranscoderListener(const std::shared_ptr<ITranscoderListener>& listener) noexcept
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

void Transcoder::startTranscoding(const Glib::ustring& uri)
{
    if (!hasStableState(State::stopped))
    {
        throw InvalidStateException();
    }

    if (m_encoders.empty())
    {
        throw NoEncoderException();
    }

    assert(m_connectors.empty()); // NOLINT

    m_prerollingPads = 1;
    m_prerollDone = false;
    m_pendingState = State::prerolled;
    m_interrupted = false;
    m_uriDecodeBin->property_uri() = uri;
    m_pipeline->set_state(Gst::STATE_PAUSED);
}

void Transcoder::stopTranscoding() noexcept
{
    if (hasStableState(State::transcoding))
    {
        m_interrupted = true;
        m_pipeline->send_event(Gst::EventEos::create());
    }
    else if (!hasStableState(State::stopped))
    {
        m_pipeline->set_state(Gst::STATE_NULL);
        disconnectEncoders();

        m_currentState = State::stopped;
        m_pendingState = State::undefined;
        triggerTranscodingStopped();
    }
}

void Transcoder::onPadAdded(const Glib::RefPtr<Gst::Pad>& pad) noexcept
{
    // WARNING: called from any streaming thread.
    if (m_prerollDone)
    {
        return;
    }

    ++this->m_prerollingPads;

    Connector connector;
    connector.decoderPad = pad;
    connector.type = GST_STREAM_TYPE_UNKNOWN;

    auto name = pad->get_current_caps()->get_structure(0).get_name();
    if (name.compare("video/x-raw") == 0)
    {
        connector.type = GST_STREAM_TYPE_VIDEO;
    }
    else if (name.compare("audio/x-raw") == 0)
    {
        connector.type = GST_STREAM_TYPE_AUDIO;
    }

    connector.outputTee = Gst::Tee::create();
    connector.outputTee->property_allow_not_linked() = true;
    auto teeSinkPad = connector.outputTee->get_static_pad("sink");
    try
    {
        m_pipeline->add(connector.outputTee);
        if (pad->link(teeSinkPad) != Gst::PAD_LINK_OK)
        {
            throw CannotLinkPadException();
        }

        if (!connector.outputTee->sync_state_with_parent())
        {
            throw InvalidStateException();
        }
    }
    catch (const std::exception& e)
    {
        connector.outputTee->set_state(Gst::STATE_NULL);
        if (connector.outputTee->has_as_parent(m_pipeline))
        {
            m_pipeline->remove(connector.outputTee);
        }

        m_pipeline->get_bus()->post(
            Gst::MessageError::create(m_uriDecodeBin,
                                      Glib::Error(errorDomain, static_cast<int>(ErrorCode::cannotInitializeOutputTee),
                                                  "cannot initialize output tee"),
                                      e.what()));
        return;
    }

    connector.blockingProbeId =
        pad->add_probe(Gst::PAD_PROBE_TYPE_BLOCK_DOWNSTREAM,
                       [this](const Glib::RefPtr<Gst::Pad>& /*pad*/, const Gst::PadProbeInfo& /*info*/) {
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
    m_connectors.push_back(std::move(connector));
}

void Transcoder::onPadPrerolled() noexcept
{
    // WARNING: called from any streaming thread.
    if (m_prerollDone)
    {
        return;
    }

    if (--m_prerollingPads <= 0)
    {
        m_prerollDone = true;
        m_pipeline->get_bus()->post(Gst::MessageApplication::create(m_uriDecodeBin, Gst::Structure("preroll_done")));
    }
}

void Transcoder::onConfigurePipeline() noexcept
{
    assert(hasStableState(State::prerolled)); // NOLINT

    if (prepareEncoders() && connectEncoders())
    {
        m_pendingState = State::transcoding;
        m_pipeline->set_state(Gst::STATE_PLAYING);
    }
}

bool Transcoder::hasStableState(State state) const noexcept
{
    return (m_currentState == state) && (m_pendingState == State::undefined);
}

bool Transcoder::onBusMessage(const Glib::RefPtr<Gst::Bus>& /*bus*/, const Glib::RefPtr<Gst::Message>& message) noexcept
{
    switch (message->get_message_type())
    {
    case Gst::MESSAGE_STATE_CHANGED: {
        auto msgStateChanged = Glib::RefPtr<Gst::MessageStateChanged>::cast_static(message);
        if ((msgStateChanged->parse_new_state() == Gst::STATE_PLAYING) && (m_pendingState == State::transcoding))
        {
            m_currentState = State::transcoding;
            m_pendingState = State::undefined;
            triggerTranscodingStarted();
        }
        break;
    }

    case Gst::MESSAGE_APPLICATION: {
        auto msgApplication = Glib::RefPtr<Gst::MessageApplication>::cast_static(message);
        Glib::ustring name = msgApplication->get_structure().get_name();
        if ((name.compare("preroll_done") == 0) && (m_pendingState == State::prerolled))
        {
            m_currentState = State::prerolled;
            m_pendingState = State::undefined;
            onConfigurePipeline();
        }
        break;
    }

    case Gst::MESSAGE_EOS:
        m_pendingState = State::stopped;
        stopTranscoding();
        break;

    case Gst::MESSAGE_ERROR: {
        Glib::Error error;
        auto msgError = Glib::RefPtr<Gst::MessageError>::cast_static(message);
        if (msgError)
        {
            error = msgError->parse_error();
        }
        else
        {
            error = Glib::Error(errorDomain, static_cast<int>(ErrorCode::undefined), "undefined error");
        }
        stopTranscoding();
        triggerTranscodingIssue(true, error);
        break;
    }

    case Gst::MESSAGE_WARNING: {
        Glib::Error error;
        auto msgWarning = Glib::RefPtr<Gst::MessageWarning>::cast_static(message);
        if (msgWarning)
        {
            error = msgWarning->parse_error();
        }
        else
        {
            error = Glib::Error(errorDomain, static_cast<int>(ErrorCode::undefined), "undefined error");
        }
        triggerTranscodingIssue(false, error);
        break;
    }

    default:
        break;
    }

    return true;
}

void Transcoder::triggerTranscodingStarted() noexcept
{
    for (auto it = m_listeners.begin(); it != m_listeners.end();)
    {
        auto listener = it->lock();
        if (listener)
        {
            listener->onTranscodingStarted(*this);
            ++it;
        }
        else
        {
            it = m_listeners.erase(it);
        }
    }
}

void Transcoder::triggerTranscodingStopped() noexcept
{
    for (auto it = m_listeners.begin(); it != m_listeners.end();)
    {
        auto listener = it->lock();
        if (listener)
        {
            listener->onTranscodingStopped(*this, m_interrupted);
            ++it;
        }
        else
        {
            it = m_listeners.erase(it);
        }
    }
}

void Transcoder::triggerTranscodingIssue(bool isFatalError, const Glib::Error& error) noexcept
{
    for (auto it = m_listeners.begin(); it != m_listeners.end();)
    {
        auto listener = it->lock();
        if (listener)
        {
            listener->onTranscodingIssue(*this, isFatalError, error);
            ++it;
        }
        else
        {
            it = m_listeners.erase(it);
        }
    }
}

bool Transcoder::prepareEncoders() noexcept
{
    assert(hasStableState(State::prerolled)); // NOLINT

    for (auto it = m_encoders.begin(); it != m_encoders.end(); ++it)
    {
        if (!(*it)->prepareEncoding(m_pipeline))
        {
            for (auto jt = m_encoders.begin(); jt != it; ++jt)
            {
                (*jt)->cleanupAfterEncoding();
            }

            return false;
        }
    }

    return true;
}

bool Transcoder::connectEncoders() noexcept
{
    assert(hasStableState(State::prerolled)); // NOLINT

    // Connect and configure encoders.
    for (auto& encoder : m_encoders)
    {
        for (auto& connector : m_connectors)
        {
            if ((connector.type & (GST_STREAM_TYPE_AUDIO | GST_STREAM_TYPE_VIDEO)) == 0)
            {
                continue;
            }

            auto pad = connector.outputTee->get_request_pad("src_%u");
            if (!pad)
            {
                disconnectEncoders();

                m_pipeline->get_bus()->post(
                    Gst::MessageError::create(m_uriDecodeBin,
                                              Glib::Error(errorDomain, static_cast<int>(ErrorCode::cannotRequestTeePad),
                                                          "cannot request output tee src pad"),
                                              ""));
                return false;
            }

            if (!encoder->linkPad(pad, ((connector.type & GST_STREAM_TYPE_VIDEO) != 0)))
            {
                disconnectEncoders();
                return false;
            }
        }

        encoder->configureEncoders();
    }

    // Unblock uridecodebin pads now that pipeline graph is complete.
    for (auto& connector : m_connectors)
    {
        connector.decoderPad->remove_probe(connector.blockingProbeId);
        connector.decoderPad.reset();
        connector.blockingProbeId = 0;
    }

    return true;
}

void Transcoder::disconnectEncoders() noexcept
{
    for (auto& encoder : m_encoders)
    {
        encoder->cleanupAfterEncoding();
    }

    for (auto& connector : m_connectors)
    {
        if (connector.decoderPad)
        {
            connector.decoderPad->remove_probe(connector.blockingProbeId);
            connector.decoderPad.reset();
            connector.blockingProbeId = 0;
        }

        if (connector.outputTee)
        {
            connector.outputTee->set_state(Gst::STATE_NULL);
            if (connector.outputTee->has_as_parent(m_pipeline))
            {
                m_pipeline->remove(connector.outputTee);
            }

            auto it = connector.outputTee->iterate_src_pads();
            while (it.next() == Gst::ITERATOR_OK)
            {
                connector.outputTee->release_request_pad(*it);
            }

            connector.outputTee.reset();
        }
    }

    m_connectors.clear();
}
