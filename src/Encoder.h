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

class Encoder
{
  public:
    enum class ErrorCode : int
    {
        undefined,
        cannotAddToPipeline,
        cannotLinkStream,
        cannotConfigureVideoEncoder,
        cannotConfigureAudioEncoder
    };
    static const GQuark errorDomain;
    static const int sameAsSource;

    Encoder();
    virtual ~Encoder();

    Encoder(const Encoder&) = delete;
    Encoder& operator=(const Encoder&) = delete;
    Encoder(Encoder&&) = default;
    Encoder& operator=(Encoder&&) = default;

    void setOutputFile(const Glib::ustring& file) noexcept;

    void setVideoDimensions(int width = sameAsSource, int height = sameAsSource) noexcept;
    void setVideoFramerate(int numerator = sameAsSource, int denominator = 1) noexcept;

    void setAudioChannels(int n = sameAsSource) noexcept;
    void setAudioSampleRate(int rate = sameAsSource) noexcept;

    bool prepareEncoding(const Glib::RefPtr<Gst::Pipeline>& pipeline) noexcept;
    bool linkPad(const Glib::RefPtr<Gst::Pad>& pad, bool isVideoPad) noexcept;
    void configureEncoders() noexcept;
    void cleanupAfterEncoding() noexcept;

  protected:
    virtual const char* getContainerType() const noexcept = 0;
    virtual const char* getVideoType() const noexcept = 0;
    virtual const char* getAudioType() const noexcept = 0;

    virtual void onConfigureVideoEncoder(const Glib::RefPtr<Gst::Element>& element);
    virtual void onConfigureAudioEncoder(const Glib::RefPtr<Gst::Element>& element);

  private:
    Glib::RefPtr<Gst::EncodeBin> m_encodeBin;
    Glib::RefPtr<Gst::FileSink> m_fileSink;

    Glib::ustring m_outputFile;

    int m_videoWidth;
    int m_videoHeight;
    int m_framerateNumerator;
    int m_framerateDenominator;
    Glib::RefPtr<Gst::Caps> getVideoCaps() const noexcept;

    int m_audioChannels;
    int m_audioSampleRate;
    Glib::RefPtr<Gst::Caps> getAudioCaps() const noexcept;

    Glib::RefPtr<Gst::EncodingProfile> createEncodingProfile() const;
};
