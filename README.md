dubby-dub
=========

[![Build](https://github.com/neodesys/dubby-dub/workflows/Build%20dubby-dub/badge.svg)](https://github.com/neodesys/dubby-dub/actions)
[![GPLv3 License](https://img.shields.io/badge/License-GPLv3-blue.svg?style=flat)](/LICENSE)

dubby-dub transcodes audio/video files into different formats using gstreamer.

**Contents**

- [Quick start](#quick-start)
  - [Build the executable](#build-the-executable)
  - [Command line interface](#command-line-interface)
  - [Example](#example)
- [How to contribute](#how-to-contribute)
- [License](#license)

--------------------------------------------------------------------------------

Quick start
-----------

Download the
[latest release](https://github.com/neodesys/dubby-dub/releases/latest) or
clone this repository.

```
$ git clone https://github.com/neodesys/dubby-dub.git
$ cd dubby-dub
```

### Build the executable

In order to build dubby-dub, you will need to install those prerequisites:
- cmake >= 3.16
- clang-format-10
- clang-tidy-10
- an ISO C++ toolchain (>= C++17)
- gstreamermm-1.0 development libraries and dependencies

On a Debian/Ubuntu (>= focal) based distribution you can install all this at once by calling:
```
$ sudo apt-get install cmake clang-format-10 clang-tidy-10 g++ libgstreamermm-1.0-dev
```

Then you can build dubby-dub by calling:
```
$ cmake -S. -Bbuild
$ cmake --build build
```

### Command line interface

In order to run dubby-dub, you will need to install those runtime libraries:
- gstreamer >= 1.14
- libgstreamermm-1.0 >= 1.10
- gstreamer1.0-plugins-good
- gstreamer1.0-plugins-bad
- gstreamer1.0-plugins-ugly
- gstreamer1.0-vaapi (if you want hardware acceleration)

On a Debian/Ubuntu (>= focal) based distribution you can install all this at once by calling:
```
$ sudo apt-get install libgstreamermm-1.0-1 gstreamer1.0-plugins-good gstreamer1.0-plugins-bad gstreamer1.0-plugins-ugly gstreamer1.0-vaapi
```

Then you can run dubby-dub:
```
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
```

### Example

```
$ ./dubby-dub -o ./ https://upload.wikimedia.org/wikipedia/commons/transcoded/c/c0/Big_Buck_Bunny_4K.webm/Big_Buck_Bunny_4K.webm.480p.vp9.webm -- {\"type\": \"transcoder\", \"encoders\": [{\"type\": \"webm\", \"width\": 640, \"video\": {\"type\": \"vp8\"}, \"audio\": {\"type\": \"vorbis\"}}]}
```

--------------------------------------------------------------------------------

How to contribute
-----------------

You are welcome to contribute to dubby-dub.

If you find a bug, have an issue or great ideas to make it better, please
[post an issue](https://guides.github.com/features/issues/).

If you can fix a bug or want to add a feature yourself, please
[fork](https://guides.github.com/activities/forking/) the repository and post a
*Pull Request*.

You can find detailed information about how to contribute in
[GitHub Guides](https://guides.github.com/activities/contributing-to-open-source/).

--------------------------------------------------------------------------------

License
-------

dubby-dub is released under the [GPLv3 License](/LICENSE).

```
dubby-dub

Copyright (C) 2020, Loïc Le Page

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.
```

--------------------------------------------------------------------------------
