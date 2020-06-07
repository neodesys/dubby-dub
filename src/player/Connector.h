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

#include <gstreamermm.h>

class Connector final
{
  public:
    Connector(const Glib::RefPtr<Gst::Pad>& srcPad, const Gst::Pad::SlotProbe& blockingProbeSlot);
    ~Connector();

    Connector(const Connector&) = delete;
    Connector& operator=(const Connector&) = delete;
    Connector(Connector&&) = default;
    Connector& operator=(Connector&&) = default;

    GstStreamType getStreamType() const noexcept
    {
        return m_streamType;
    }

    void connect(const Glib::RefPtr<Gst::Pad>& sinkPad);
    void unblock() noexcept;

  private:
    Glib::RefPtr<Gst::Tee> m_outputTee;
    Glib::RefPtr<Gst::Pad> m_srcPad;
    unsigned long m_blockingProbeId;
    GstStreamType m_streamType;
};
