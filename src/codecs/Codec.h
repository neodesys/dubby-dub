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
#pragma once

#include "../ISerializable.h"
#include <gstreamermm.h>

class Codec : public ISerializable
{
  public:
    static constexpr int defaultValue = -1;

    static std::shared_ptr<Codec> createCodec(const std::string& type);
    static void forceSoftwareEncoding(bool force) noexcept;

    Codec() = default;

    Codec(const Codec&) = delete;
    Codec& operator=(const Codec&) = delete;
    Codec(Codec&&) = default;
    Codec& operator=(Codec&&) = default;

    Json serialize() const override;
    void unserialize(const Json& in) override;

    virtual const char* getType() const noexcept = 0;
    virtual const char* getMimeType() const noexcept = 0;
    virtual void configureElement(const Glib::ustring& factoryName,
                                  const Glib::RefPtr<Gst::Element>& element) const = 0;
};
