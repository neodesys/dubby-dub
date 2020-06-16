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
#include "Transcoder.h"
#include "exceptions.h"
#include <iostream>

namespace
{
constexpr const char* encodersKey = "encoders";
} // namespace

std::shared_ptr<Transcoder> Transcoder::create(int argc, char** argv, bool forceSoftwareEncoding)
{
    Gst::init(argc, argv);
    Codec::forceSoftwareEncoding(forceSoftwareEncoding);
    std::shared_ptr<Transcoder> transcoder(new Transcoder());
    transcoder->m_player.addPlayerListener(transcoder);
    return transcoder;
}

Transcoder::Transcoder()
{
    m_mainLoop = Glib::MainLoop::create();

    if (!m_mainLoop)
    {
        throw UnrecoverableError();
    }
}

void Transcoder::addEncoder(const std::shared_ptr<Encoder>& encoder)
{
    if (!m_player.hasStableState(Player::State::stopped))
    {
        throw InvalidStateException();
    }

    if (encoder && (std::find(m_encoders.begin(), m_encoders.end(), encoder) == m_encoders.end()))
    {
        m_player.addPlayerListener(encoder);
        m_encoders.push_back(encoder);
    }
}

void Transcoder::clearEncoders()
{
    if (!m_player.hasStableState(Player::State::stopped))
    {
        throw InvalidStateException();
    }

    for (auto& encoder : m_encoders)
    {
        m_player.removePlayerListener(encoder);
    }

    m_encoders.clear();
}

void Transcoder::transcode(const Glib::ustring& uri)
{
    if (m_encoders.empty())
    {
        throw NoEncoderException();
    }

    m_player.play(uri);
    m_mainLoop->run();
}

void Transcoder::interruptTranscoding() noexcept
{
    std::cout << "Interrupting transcoding..." << std::endl;
    m_player.stop();
}

void Transcoder::onPlayerPrerolled(Player& /*player*/)
{
    std::cout << "Configuring transcoder..." << std::endl;
}

void Transcoder::onPlayerPlaying(Player& /*player*/) noexcept
{
    std::cout << "Transcoding..." << std::endl;
}

void Transcoder::onPlayerStopped(Player& /*player*/, bool isInterrupted) noexcept
{
    if (isInterrupted)
    {
        std::cout << "Transcoding interrupted before end" << std::endl;
    }
    else
    {
        std::cout << "Transcoding finished" << std::endl;
    }

    m_mainLoop->quit();
}

void Transcoder::onPipelineIssue(Player& /*player*/, bool isFatalError, const Glib::Error& error,
                                 const std::string& debugMessage) noexcept
{
    std::cerr << "Transcoding issue: " << error.what() << " (Debug info: " << debugMessage << ")" << std::endl;
    if (isFatalError)
    {
        std::cerr << "Error is fatal, stopping transcoder..." << std::endl;
    }
}

Json Transcoder::serialize() const
{
    Json encoders = Json::array();
    for (const auto& encoder : m_encoders)
    {
        encoders.push_back(encoder->serialize());
    }

    Json obj = Json::object();
    obj[ISerializable::typeKey] = Transcoder::type;
    obj[encodersKey] = std::move(encoders);
    return obj;
}

void Transcoder::unserialize(const Json& in)
{
    if (in.at(ISerializable::typeKey).get<std::string>() != Transcoder::type)
    {
        throw InvalidTypeException();
    }

    clearEncoders();
    for (const auto& entry : in.at(encodersKey))
    {
        auto encoder = Encoder::createEncoder(entry.at(ISerializable::typeKey).get<std::string>());
        encoder->unserialize(entry);
        addEncoder(encoder);
    }
}
