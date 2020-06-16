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
#include "H264Codec.h"
#include "../../exceptions.h"
#include <algorithm>

namespace
{
constexpr int minBitrate = 1;
constexpr int maxBitrate = 102400;
constexpr int defaultBitrate = 2048;

constexpr int minQP = 1;
constexpr int maxQP = 51;
constexpr int defaultQP = 26;
} // namespace

void H264Codec::forceSoftwareEncoding(bool force) noexcept
{
    auto registry = Gst::Registry::get();
    auto soft = registry->lookup_feature("x264enc");
    if (soft)
    {
        soft->set_rank(force ? Gst::RANK_PRIMARY : Gst::RANK_SECONDARY);
    }

    auto hard = registry->lookup_feature("vaapih264enc");
    if (hard)
    {
        hard->set_rank(force ? Gst::RANK_SECONDARY : Gst::RANK_PRIMARY);
    }
}

const char* H264Codec::getMimeType() const noexcept
{
    return "video/x-h264,profile=main";
}

void H264Codec::configureElement(const Glib::ustring& factoryName, const Glib::RefPtr<Gst::Element>& element) const
{
    const int bitrate =
        (m_bitrateInKbps != defaultValue) ? std::clamp(m_bitrateInKbps, minBitrate, maxBitrate) : defaultBitrate;
    const int qp =
        (m_qualityInPercent != defaultValue)
            ? minQP + static_cast<int>((maxQP - minQP) * (1.F - static_cast<float>(m_qualityInPercent) / 100.F))
            : defaultQP;

    if (factoryName == "x264enc")
    {
        switch (m_encodingMode)
        {
        case EncodingMode::defaultValue:
        case EncodingMode::bitrate:
            element->set_property("pass", 0);
            element->set_property("bitrate", bitrate);
            element->set_property("quantizer", defaultQP - 1);
            break;

        case EncodingMode::quality:
            element->set_property("pass", 5);
            element->set_property("bitrate", defaultBitrate);
            element->set_property("quantizer", qp - 1);
            break;
        }
    }
    else if (factoryName == "vaapih264enc")
    {
        element->set_property("keyframe-period", 0);
        switch (m_encodingMode)
        {
        case EncodingMode::defaultValue:
        case EncodingMode::bitrate:
            element->set_property("rate-control", 4);
            element->set_property("bitrate", bitrate);
            element->set_property("init-qp", defaultQP);
            break;

        case EncodingMode::quality:
            element->set_property("rate-control", 1);
            element->set_property("bitrate", 0);
            element->set_property("init-qp", qp);
            break;
        }
    }
    else
    {
        throw UnknownCodecElementException();
    }
}
