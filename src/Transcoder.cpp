#include "Transcoder.h"
#include "exceptions.h"
#include <iostream>

std::shared_ptr<Transcoder> Transcoder::create(int argc, char** argv)
{
    Gst::init(argc, argv);
    std::shared_ptr<Transcoder> transcoder(new Transcoder());
    transcoder->m_player.addPlayerListener(static_cast<std::shared_ptr<IPlayerListener>>(transcoder));
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
