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
#include "VorbisCodec.h"
#include "../../exceptions.h"
#include <algorithm>

namespace
{
constexpr int minBitrate = 1;
constexpr int maxBitrate = 250;
constexpr int defaultBitrate = 64;

constexpr float minQP = 0.1F;
constexpr float maxQP = 1.F;
constexpr float defaultQP = 0.3F;
} // namespace

const char* VorbisCodec::getMimeType() const noexcept
{
    return "audio/x-vorbis";
}

void VorbisCodec::configureElement(const Glib::ustring& factoryName, const Glib::RefPtr<Gst::Element>& element) const
{
    const int bitrate =
        (m_bitrateInKbps != defaultValue) ? std::clamp(m_bitrateInKbps, minBitrate, maxBitrate) : defaultBitrate;
    const float qp = (m_qualityInPercent != defaultValue)
                         ? minQP + (maxQP - minQP) * static_cast<float>(m_qualityInPercent) / 100.F
                         : defaultQP;

    if (factoryName == "vorbisenc")
    {
        switch (m_encodingMode)
        {
        case EncodingMode::bitrate:
            element->set_property("bitrate", bitrate * 1000);
            element->set_property("quality", defaultQP);
            break;

        case EncodingMode::defaultValue:
        case EncodingMode::quality:
            element->set_property("bitrate", -1);
            element->set_property("quality", qp);
            break;
        }
    }
    else
    {
        throw UnknownCodecElementException();
    }
}
