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
#include <iostream>

int main(int argc, char* argv[])
{
    try
    {
        auto transcoder = Transcoder::create(argc, argv);

        auto stdinChannel = Glib::IOChannel::create_from_fd(fileno(stdin));
        Glib::signal_io().connect(
            [&stdinChannel, &transcoder](Glib::IOCondition /*condition*/) {
                Glib::ustring line;
                if (stdinChannel->read_line(line) == Glib::IO_STATUS_NORMAL)
                {
                    if (line.at(0) == 'q')
                    {
                        transcoder->interruptTranscoding();
                    }
                }

                return true;
            },
            stdinChannel, Glib::IO_IN);

        auto config = R"({
            "type": "transcoder",
            "encoders": [
                {
                    "type": "webm",
                    "file": "./out.webm",
                    "width": 640,
                    "framerate": 25,
                    "channels": 2,
                    "samplerate": 44100,
                    "video": {
                        "type": "vp9",
                        "mode": "quality",
                        "quality": 80
                    },
                    "audio": {
                        "type": "vorbis",
                        "quality": 100
                    }
                },
                {
                    "type": "mp4",
                    "file": "./out.mp4",
                    "width": 320,
                    "framerate": [50, 3],
                    "channels": 1,
                    "samplerate": 22050,
                    "video": {
                        "type": "h264",
                        "mode": "quality",
                        "quality": 50
                    },
                    "audio": {
                        "type": "aac",
                        "bitrate": 56
                    }
                }
            ]
        })"_json;
        transcoder->unserialize(config);

        transcoder->transcode("https://upload.wikimedia.org/wikipedia/commons/transcoded/c/c0/Big_Buck_Bunny_4K.webm/"
                              "Big_Buck_Bunny_4K.webm.480p.vp9.webm");

        std::cout << "Exiting..." << std::endl;
        return 0;
    }
    catch (const std::exception& e)
    {
        std::cerr << "System error: " << e.what() << std::endl;
        return 1;
    }
}
