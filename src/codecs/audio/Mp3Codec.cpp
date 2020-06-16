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
#include "Mp3Codec.h"
#include "../../exceptions.h"
#include <algorithm>

namespace
{
constexpr int minBitrate = 8;
constexpr int maxBitrate = 320;
constexpr int defaultBitrate = 128;

constexpr float minQP = 0.F;
constexpr float maxQP = 9.999F;
constexpr float defaultQP = 4.F;
} // namespace

const char* Mp3Codec::getMimeType() const noexcept
{
    return "audio/mpeg,mpegversion=1,layer=3";
}

void Mp3Codec::configureElement(const Glib::ustring& factoryName, const Glib::RefPtr<Gst::Element>& element) const
{
    const int bitrate =
        (m_bitrateInKbps != defaultValue) ? std::clamp(m_bitrateInKbps, minBitrate, maxBitrate) : defaultBitrate;
    const float qp = (m_qualityInPercent != defaultValue)
                         ? minQP + (maxQP - minQP) * (1.F - static_cast<float>(m_qualityInPercent) / 100.F)
                         : defaultQP;

    if (factoryName == "lamemp3enc")
    {
        switch (m_encodingMode)
        {
        case EncodingMode::bitrate:
            element->set_property("target", 1);
            element->set_property("bitrate", bitrate);
            element->set_property("quality", defaultQP);
            break;

        case EncodingMode::defaultValue:
        case EncodingMode::quality:
            element->set_property("target", 0);
            element->set_property("bitrate", defaultBitrate);
            element->set_property("quality", qp);
            break;
        }
    }
    else
    {
        throw UnknownCodecElementException();
    }
}
