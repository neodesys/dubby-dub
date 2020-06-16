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
#include "Vp9Codec.h"
#include "../../exceptions.h"
#include <algorithm>

namespace
{
constexpr int minBitrate = 1;
constexpr int maxBitrate = 102400;
constexpr int defaultBitrate = 2048;

constexpr int minQP = 1;
constexpr int maxQP = 7;
constexpr int defaultQP = 2;
} // namespace

void Vp9Codec::forceSoftwareEncoding(bool force) noexcept
{
    auto registry = Gst::Registry::get();
    auto soft = registry->lookup_feature("vp9enc");
    if (soft)
    {
        soft->set_rank(force ? Gst::RANK_PRIMARY : Gst::RANK_SECONDARY);
    }

    auto hard = registry->lookup_feature("vaapivp9enc");
    if (hard)
    {
        hard->set_rank(force ? Gst::RANK_SECONDARY : Gst::RANK_PRIMARY);
    }
}

const char* Vp9Codec::getMimeType() const noexcept
{
    return "video/x-vp9";
}

void Vp9Codec::configureElement(const Glib::ustring& factoryName, const Glib::RefPtr<Gst::Element>& element) const
{
    const int bitrate =
        (m_bitrateInKbps != defaultValue) ? std::clamp(m_bitrateInKbps, minBitrate, maxBitrate) : defaultBitrate;
    const int qp =
        (m_qualityInPercent != defaultValue)
            ? minQP + static_cast<int>((maxQP - minQP) * (1.F - static_cast<float>(m_qualityInPercent) / 100.F))
            : defaultQP;

    if (factoryName == "vp9enc")
    {
        switch (m_encodingMode)
        {
        case EncodingMode::defaultValue:
        case EncodingMode::bitrate:
            element->set_property("end-usage", 0);
            element->set_property("target-bitrate", bitrate * 1000);
            element->set_property("cq-level", 10 * (defaultQP - 1));
            break;

        case EncodingMode::quality:
            element->set_property("end-usage", 2);
            element->set_property("target-bitrate", 0);
            element->set_property("cq-level", 10 * (qp - 1));
            break;
        }
    }
    else if (factoryName == "vaapivp9enc")
    {
        element->set_property("keyframe-period", 0);
        switch (m_encodingMode)
        {
        case EncodingMode::defaultValue:
        case EncodingMode::bitrate:
            element->set_property("rate-control", 4);
            element->set_property("bitrate", bitrate);
            element->set_property("quality-level", qp);
            break;

        case EncodingMode::quality:
            element->set_property("rate-control", 1);
            element->set_property("bitrate", 0);
            element->set_property("quality-level", qp);
            break;
        }
    }
    else
    {
        throw UnknownCodecElementException();
    }
}
