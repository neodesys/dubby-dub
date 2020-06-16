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
#include "BitrateCodec.h"

namespace
{
constexpr const char* bitrateInKbpsKey = "bitrate";
} // namespace

BitrateCodec::BitrateCodec() : m_bitrateInKbps(defaultValue)
{
    // Empty constructor.
}

void BitrateCodec::setBitrate(int kbps) noexcept
{
    m_bitrateInKbps = (kbps >= 0) ? kbps : defaultValue;
}

Json BitrateCodec::serialize() const
{
    Json obj = Codec::serialize();

    if (m_bitrateInKbps != defaultValue)
    {
        obj[bitrateInKbpsKey] = m_bitrateInKbps;
    }

    return obj;
}

void BitrateCodec::unserialize(const Json& in)
{
    Codec::unserialize(in);

    int bitrate = defaultValue;
    if (in.contains(bitrateInKbpsKey))
    {
        bitrate = in.at(bitrateInKbpsKey).get<int>();
    }
    setBitrate(bitrate);
}
