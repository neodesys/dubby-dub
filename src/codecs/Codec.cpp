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
#include "audio/AacCodec.h"
#include "audio/Mp3Codec.h"
#include "audio/OpusCodec.h"
#include "audio/VorbisCodec.h"
#include "video/H264Codec.h"
#include "video/H265Codec.h"
#include "video/TheoraCodec.h"
#include "video/Vp8Codec.h"
#include "video/Vp9Codec.h"

std::shared_ptr<Codec> Codec::createCodec(const std::string& type)
{
    if (type == H264Codec::type)
    {
        return std::make_shared<H264Codec>();
    }

    if (type == H265Codec::type)
    {
        return std::make_shared<H265Codec>();
    }

    if (type == TheoraCodec::type)
    {
        return std::make_shared<TheoraCodec>();
    }

    if (type == Vp8Codec::type)
    {
        return std::make_shared<Vp8Codec>();
    }

    if (type == Vp9Codec::type)
    {
        return std::make_shared<Vp9Codec>();
    }

    if (type == AacCodec::type)
    {
        return std::make_shared<AacCodec>();
    }

    if (type == Mp3Codec::type)
    {
        return std::make_shared<Mp3Codec>();
    }

    if (type == OpusCodec::type)
    {
        return std::make_shared<OpusCodec>();
    }

    if (type == VorbisCodec::type)
    {
        return std::make_shared<VorbisCodec>();
    }

    throw InvalidTypeException();
}

void Codec::forceSoftwareEncoding(bool force) noexcept
{
    H264Codec::forceSoftwareEncoding(force);
    H265Codec::forceSoftwareEncoding(force);
    Vp8Codec::forceSoftwareEncoding(force);
    Vp9Codec::forceSoftwareEncoding(force);
}

Json Codec::serialize() const
{
    Json obj = Json::object();
    obj[ISerializable::typeKey] = getType();
    return obj;
}

void Codec::unserialize(const Json& in)
{
    if (in.at(ISerializable::typeKey).get<std::string>() != getType())
    {
        throw InvalidTypeException();
    }
}
