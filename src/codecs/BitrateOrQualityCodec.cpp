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
#include "BitrateOrQualityCodec.h"
#include <algorithm>

namespace
{
constexpr const char* encodingModeKey = "mode";
constexpr const char* qualityInPercentKey = "quality";
} // namespace

NLOHMANN_JSON_SERIALIZE_ENUM(BitrateOrQualityCodec::EncodingMode, // NOLINT
                             {{BitrateOrQualityCodec::EncodingMode::defaultValue, ""},
                              {BitrateOrQualityCodec::EncodingMode::bitrate, "bitrate"},
                              {BitrateOrQualityCodec::EncodingMode::quality, "quality"}});

BitrateOrQualityCodec::BitrateOrQualityCodec()
    : m_encodingMode(EncodingMode::defaultValue), m_qualityInPercent(defaultValue)
{
    // Empty constructor.
}

void BitrateOrQualityCodec::setEncodingMode(EncodingMode mode) noexcept
{
    m_encodingMode = mode;
}

void BitrateOrQualityCodec::setQuality(int percent) noexcept
{
    m_qualityInPercent = (percent >= 0) ? std::min(percent, 100) : defaultValue;
}

Json BitrateOrQualityCodec::serialize() const
{
    Json obj = BitrateCodec::serialize();

    if (m_encodingMode != EncodingMode::defaultValue)
    {
        obj[encodingModeKey] = m_encodingMode;
    }

    if (m_qualityInPercent != defaultValue)
    {
        obj[qualityInPercentKey] = m_qualityInPercent;
    }

    return obj;
}

void BitrateOrQualityCodec::unserialize(const Json& in)
{
    BitrateCodec::unserialize(in);

    EncodingMode mode = EncodingMode::defaultValue;
    if (in.contains(encodingModeKey))
    {
        mode = in.at(encodingModeKey).get<EncodingMode>();
    }
    setEncodingMode(mode);

    int quality = defaultValue;
    if (in.contains(qualityInPercentKey))
    {
        quality = in.at(qualityInPercentKey).get<int>();
    }
    setQuality(quality);
}
