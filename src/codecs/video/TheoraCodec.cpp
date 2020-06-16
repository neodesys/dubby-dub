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
#include "TheoraCodec.h"
#include "../../exceptions.h"
#include <algorithm>

namespace
{
constexpr int minBitrate = 1;
constexpr int maxBitrate = 16777215;
constexpr int defaultBitrate = 2048;

constexpr int minQP = 0;
constexpr int maxQP = 63;
constexpr int defaultQP = 48;
} // namespace

const char* TheoraCodec::getMimeType() const noexcept
{
    return "video/x-theora";
}

void TheoraCodec::configureElement(const Glib::ustring& factoryName, const Glib::RefPtr<Gst::Element>& element) const
{
    const int bitrate =
        (m_bitrateInKbps != defaultValue) ? std::clamp(m_bitrateInKbps, minBitrate, maxBitrate) : defaultBitrate;
    const int qp =
        (m_qualityInPercent != defaultValue)
            ? minQP + static_cast<int>((maxQP - minQP) * (1.F - static_cast<float>(m_qualityInPercent) / 100.F))
            : defaultQP;

    if (factoryName == "theoraenc")
    {
        switch (m_encodingMode)
        {
        case EncodingMode::defaultValue:
        case EncodingMode::bitrate:
            element->set_property("bitrate", bitrate);
            element->set_property("quality", defaultQP);
            break;

        case EncodingMode::quality:
            element->set_property("bitrate", 0);
            element->set_property("quality", qp);
            break;
        }
    }
    else
    {
        throw UnknownCodecElementException();
    }
}
