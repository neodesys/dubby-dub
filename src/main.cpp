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
#include <cstdio>
#include <fstream>
#include <glibmm.h>
#include <iomanip>
#include <iostream>

extern const char* dubbyDubVersion;

namespace
{
constexpr const char* help = R"(
Usage:
  dubby-dub [options] [File...]...[URI...]

  Options:
    -c/--config [File]:    read transcoder configuration from [File]
                           (see below for configuration format).
    --:                    stop command line arguments here, the rest of the
                           command line will be interpreted as json
                           configuration for the transcoder. You must provide
                           a valid configuration either inline or by using a
                           configuraton file (provided with -c [File]). If
                           inline configuration and configuration file are
                           both provided, transcoder configuration will be
                           read from the file.
    -o/--output-dir [Dir]: use [Dir] as transcoded files output directory.
                           If specified, it will override output paths from
                           json configuration. When processing more than one
                           source media, you should specify an output directory
                           in order to not override output files from previous
                           transcoding.
    -h or --help:          displays this help content and exits.
    -v or --version:       displays version and exits.

  [File...]...[URI...]:    a list of source media files (paths can be relative
                           to current working directory) and/or URIs. All
                           specified media will be transcoded sequencially one
                           after the other using the same transcoding
                           configuration.

  Configuration format (json):
  {
    "type": "transcoder",    --> compulsory to identify the configuration
    "encoders": [            --> list of encoders (one entry per transcoded
    {                            output), there must be at least one encoder
      "type": "webm",        --> encoder type (mkv|mp4|ogg|webm) (see below
                                 for codecs compatibility)
      "file": "./out.webm",  --> (optional) transcoded file output path, can
                                 be overridden by -o option
      "width": 1920,         --> (optional) output width (set <= 0 or nothing
                                 to keep input media width)
      "height": 1080,        --> (optional) output height (set <= 0 or nothing
                                 to keep input media height)
      "framerate": 25,       --> (optional) output frame rate (set <= 0 or
                                 nothing to keep input media frame rate), you
                                 can set fractional frame rates using [100, 3]
                                 for 33.3333... frames per second for example
      "channels": 2,         --> (optional) output number of audio channels
                                 (set <= 0 or nothing to keep input media
                                 number of channels)
      "samplerate": 44100,   --> (optional) output audio sample rate (set <= 0
                                 or nothing to keep input media sample rate)
      "video": {             --> (optional) output video codec, if not
                                 specified video will not be transcoded
        "type": "vp9",       --> video codec type (h264|h265|theora|vp8|vp9)
                                 (see below for codecs compatibility)
        "mode": "bitrate",   --> (optional) encoding mode (bitrate|quality),
                                 if not specified default mode is bitrate
        "bitrate": 2500,     --> (optional) encoding bitrate in kbps (only used
                                 if mode is bitrate), if negative or not
                                 specified default bitrate is 2048 kbps
        "quality": 75        --> (optional) encoding quality in percent
                                 (between 0 and 100, only used if mode is
                                 quality), if negative or not specified
                                 default quality depends on codec
      },
      "audio": {             --> (optional) output audio codec, if not
                                 specified audio will not be transcoded
        "type": "vorbis",    --> audio codec type (aac|mp3|opus|vorbis)
                                 (see below for codecs compatibility)
        "mode": "quality",   --> (optional) ONLY FOR mp3 and vorbis: encoding
                                 mode (bitrate|quality), if not specified
                                 default mode is quality
        "quality": 75,       --> (optional) ONLY FOR mp3 and vorbis: encoding
                                 quality in percent (between 0 and 100, only
                                 used if mode is quality), if negative or not
                                 specified default quality depends on codec
        "bitrate": 64        --> (optional) encoding bitrate in kbps (only used
                                 if mode is bitrate or codec is aac or opus),
                                 if negative or not specified default bitrate
                                 depends on codec
      }
    }]
  }

  Codecs compatibility:
    /-----------------------------------------\
    | Encoder |  mkv  |  mp4  |  ogg  |  webm |
    |  Codec  |       |       |       |       |
    -------------------------------------------
    |  h264   |   X   |   X   |       |       |
    |  h265   |   X   |   X   |       |       |
    | theora  |   X   |       |   X   |       |
    |   vp8   |   X   |       |       |   X   |
    |   vp9   |   X   |       |       |   X   |
    -------------------------------------------
    |   aac   |   X   |   X   |       |       |
    |   mp3   |   X   |   X   |       |       |
    |   opus  |   X   |       |   X   |   X   |
    | vorbis  |   X   |       |   X   |   X   |
    \-----------------------------------------/

  Application controls:
    At any moment you can press:
    - c<enter> to display current configuration
    - q<enter> to stop transcoding and quit the application
               (when interrupting transcoding you may have to wait a little
               bit that all buffers are correctly flushed to output files
               before application effectively exits, particularly if not
               using hardware acceleration)

  Example:
    ./dubby-dub -o ./ https://upload.wikimedia.org/wikipedia/commons/transcoded/c/c0/Big_Buck_Bunny_4K.webm/Big_Buck_Bunny_4K.webm.480p.vp9.webm -- {\"type\": \"transcoder\", \"encoders\": [{\"type\": \"webm\", \"width\": 640, \"video\": {\"type\": \"vp8\"}, \"audio\": {\"type\": \"vorbis\"}}]}
)";

struct Config
{
    Json transcoderConfig;
    std::string outputPath;
    std::vector<Glib::ustring> sourceUris;
    bool mustExit = false;
};

void printHelp()
{
    std::cout << help << std::endl;
}

void printVersion()
{
    std::cout << "dubby-dub version: " << dubbyDubVersion << std::endl;
}

Config parseConfig(int argc, char** argv)
{
    Config cfg;
    for (int i = 1; i < argc; ++i)
    {
        if (argv[i][0] == '-') // NOLINT
        {
            if (strcmp(argv[i], "--") == 0) // NOLINT
            {
                if (cfg.transcoderConfig.empty())
                {
                    std::string content;
                    for (++i; i < argc; ++i)
                    {
                        content += argv[i]; // NOLINT
                        content += " ";
                    }

                    if (!content.empty())
                    {
                        cfg.transcoderConfig = Json::parse(content);
                    }
                }
                break;
            }
            else if (((strcmp(argv[i], "-c") == 0) || (strcmp(argv[i], "--config") == 0)) && (++i < argc)) // NOLINT
            {
                std::ifstream in(argv[i]); // NOLINT
                in >> cfg.transcoderConfig;
            }
            else if (((strcmp(argv[i], "-o") == 0) || (strcmp(argv[i], "--output-dir") == 0)) && (++i < argc)) // NOLINT
            {
                cfg.outputPath = argv[i]; // NOLINT
            }
            else if ((strcmp(argv[i], "-h") == 0) || (strcmp(argv[i], "--help") == 0)) // NOLINT
            {
                cfg.mustExit = true;
                printHelp();
                break;
            }
            else if ((strcmp(argv[i], "-v") == 0) || (strcmp(argv[i], "--version") == 0)) // NOLINT
            {
                cfg.mustExit = true;
                printVersion();
                break;
            }
        }
        else if (Glib::file_test(argv[i], Glib::FILE_TEST_IS_REGULAR | Glib::FILE_TEST_EXISTS)) // NOLINT
        {
            if (!Glib::path_is_absolute(argv[i])) // NOLINT
            {
                std::string path = Glib::build_filename(Glib::get_current_dir(), argv[i]); // NOLINT
                cfg.sourceUris.push_back(Glib::filename_to_uri(path));
            }
            else
            {
                cfg.sourceUris.push_back(Glib::filename_to_uri(argv[i])); // NOLINT
            }
        }
        else
        {
            cfg.sourceUris.emplace_back(argv[i]); // NOLINT
        }
    }

    return cfg;
}
} // namespace

int main(int argc, char* argv[])
{
    try
    {
        const Config config = parseConfig(argc, argv);
        if (config.mustExit)
        {
            return 0;
        }

        auto transcoder = Transcoder::create(argc, argv);
        transcoder->unserialize(config.transcoderConfig);

        bool quit = false;
        auto stdinChannel = Glib::IOChannel::create_from_fd(fileno(stdin));
        Glib::signal_io().connect(
            [&stdinChannel, &transcoder, &quit](Glib::IOCondition /*condition*/) {
                Glib::ustring line;
                if (stdinChannel->read_line(line) == Glib::IO_STATUS_NORMAL)
                {
                    if (line.at(0) == 'c')
                    {
                        std::cout << "Current configuration:" << std::endl;
                        std::cout << std::setw(2) << transcoder->serialize() << std::endl;
                    }
                    else if (line.at(0) == 'q')
                    {
                        quit = true;
                        transcoder->interruptTranscoding();
                    }
                }

                return true;
            },
            stdinChannel, Glib::IO_IN);

        Glib::signal_timeout().connect(
            [&transcoder]() {
                float progress = transcoder->getProgress();
                if (progress > 0.F)
                {
                    std::cout << std::fixed << std::setprecision(2) << std::setw(6) << progress * 100.F << "%  \r"
                              << std::flush;
                }

                return true;
            },
            500);

        for (const auto& uri : config.sourceUris)
        {
            if (quit)
            {
                break;
            }

            if (!config.outputPath.empty())
            {
                auto name = Glib::path_get_basename(uri);
                name = name.substr(0, name.find_last_of('.'));
                name = Glib::build_filename(config.outputPath, name);
                name += "_transcoded_";

                int index = 0;
                std::ostringstream out(std::ios_base::app);
                for (const auto& encoder : transcoder->getEncoders())
                {
                    out.str(name);
                    out << std::setw(2) << std::setfill('0') << index++ << '.' << encoder->getType();
                    encoder->setOutputFile(out.str());
                }
            }

            std::cout << "Start transcoding " << uri << "..." << std::endl;
            transcoder->transcode(uri);
        }

        std::cout << "Exiting..." << std::endl;
        return 0;
    }
    catch (const std::exception& e)
    {
        std::cerr << "System error: " << e.what() << std::endl;
        printHelp();
        return 1;
    }
}
