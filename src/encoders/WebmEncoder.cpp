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
#include "WebmEncoder.h"
#include "../exceptions.h"

const char* const WebmEncoder::type = "webm";

Json WebmEncoder::serialize() const
{
    Json obj = Encoder::serialize();
    obj[ISerializable::typeKey] = WebmEncoder::type;
    return obj;
}

void WebmEncoder::unserialize(const Json& in)
{
    if (in.at(ISerializable::typeKey).get<std::string>() != WebmEncoder::type)
    {
        throw InvalidTypeException();
    }

    Encoder::unserialize(in);
}

const char* WebmEncoder::getContainerType() const noexcept
{
    return "video/webm";
}

const char* WebmEncoder::getVideoType() const noexcept
{
    return "video/x-vp9";
}

const char* WebmEncoder::getAudioType() const noexcept
{
    return "audio/x-vorbis";
}
