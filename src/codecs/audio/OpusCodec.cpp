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
#include "OpusCodec.h"
#include "../../exceptions.h"
#include <algorithm>

namespace
{
constexpr int minBitrate = 4;
constexpr int maxBitrate = 650;
constexpr int defaultBitrate = 64;
} // namespace

const char* OpusCodec::getMimeType() const noexcept
{
    return "audio/x-opus";
}

void OpusCodec::configureElement(const Glib::ustring& factoryName, const Glib::RefPtr<Gst::Element>& element) const
{
    const int bitrate =
        (m_bitrateInKbps != defaultValue) ? std::clamp(m_bitrateInKbps, minBitrate, maxBitrate) : defaultBitrate;

    if (factoryName == "opusenc")
    {
        element->set_property("bitrate-type", 1);
        element->set_property("bitrate", bitrate * 1000);
    }
    else
    {
        throw UnknownCodecElementException();
    }
}
