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
#include "Mp4Encoder.h"
#include "../codecs/audio/AacCodec.h"
#include "../codecs/audio/Mp3Codec.h"
#include "../codecs/video/H264Codec.h"
#include "../codecs/video/H265Codec.h"
#include <cstring>

const char* Mp4Encoder::getMimeType() const noexcept
{
    return "video/quicktime,variant=iso";
}

bool Mp4Encoder::isVideoCodecAccepted(const char* codecType) const noexcept
{
    return ((std::strcmp(codecType, H264Codec::type) == 0) || (std::strcmp(codecType, H265Codec::type) == 0));
}

bool Mp4Encoder::isAudioCodecAccepted(const char* codecType) const noexcept
{
    return ((std::strcmp(codecType, AacCodec::type) == 0) || (std::strcmp(codecType, Mp3Codec::type) == 0));
}
