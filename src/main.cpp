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
#include "WebmEncoder.h"
#include <cstdio>
#include <iostream>

class Application final : public ITranscoderListener
{
  public:
    static std::shared_ptr<Application> create(int argc, char** argv) noexcept;

    void onTranscodingStarted(Transcoder& transcoder) noexcept final;
    void onTranscodingStopped(Transcoder& transcoder, bool isInterrupted) noexcept final;
    void onTranscodingIssue(Transcoder& transcoder, bool isFatalError, const Glib::Error& error) noexcept final;

    void run();

  private:
    Application();
    Transcoder m_transcoder;
    Glib::RefPtr<Glib::MainLoop> m_mainLoop;
    Glib::RefPtr<Glib::IOChannel> m_stdinChannel;
};

std::shared_ptr<Application> Application::create(int argc, char** argv) noexcept
{
    Gst::init(argc, argv);
    std::shared_ptr<Application> app(new Application());
    app->m_transcoder.addTranscoderListener(app);
    return app;
}

Application::Application()
{
    m_mainLoop = Glib::MainLoop::create();
    m_stdinChannel = Glib::IOChannel::create_from_fd(fileno(stdin));

    Glib::signal_io().connect(
        [this](Glib::IOCondition /*condition*/) {
            Glib::ustring line;
            if (this->m_stdinChannel->read_line(line) == Glib::IO_STATUS_NORMAL)
            {
                if (line.at(0) == 'q')
                {
                    std::cout << "Interrupting transcoding..." << std::endl;
                    this->m_transcoder.stopTranscoding();
                }
            }

            return true;
        },
        m_stdinChannel, Glib::IO_IN);
}

void Application::onTranscodingStarted(Transcoder& /*transcoder*/) noexcept
{
    std::cout << "Transcoding started..." << std::endl;
}

void Application::onTranscodingStopped(Transcoder& /*transcoder*/, bool isInterrupted) noexcept
{
    if (isInterrupted)
    {
        std::cout << "Transcoding interrupted before end." << std::endl;
    }
    else
    {
        std::cout << "Transcoding finished." << std::endl;
    }
    m_mainLoop->quit();
}

void Application::onTranscodingIssue(Transcoder& /*transcoder*/, bool isFatalError, const Glib::Error& error) noexcept
{
    std::cerr << "Transcoding issue: " << error.what() << std::endl;
    if (isFatalError)
    {
        std::cerr << "Error is fatal, transcoder stopping..." << std::endl;
    }
}

void Application::run()
{
    auto encoder = std::make_shared<WebmEncoder>();
    m_transcoder.addEncoder(encoder);

    encoder->setOutputFile("./out.webm");
    m_transcoder.startTranscoding("https://upload.wikimedia.org/wikipedia/commons/transcoded/c/c0/"
                                  "Big_Buck_Bunny_4K.webm/Big_Buck_Bunny_4K.webm.480p.vp9.webm");
    m_mainLoop->run();
}

int main(int argc, char* argv[])
{
    try
    {
        auto app = Application::create(argc, argv);
        app->run();

        std::cout << "Exiting..." << std::endl;
        return 0;
    }
    catch (const std::exception& e)
    {
        std::cerr << "Unrecoverable error: " << e.what() << std::endl;
        return 1;
    }
}
