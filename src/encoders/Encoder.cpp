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
#include "../exceptions.h"
#include "Mp4Encoder.h"
#include "WebmEncoder.h"

namespace
{
constexpr const char* outputFileKey = "file";
constexpr const char* videoWidthKey = "width";
constexpr const char* videoHeightKey = "height";
constexpr const char* frameRateKey = "framerate";
constexpr const char* audioChannelsKey = "channels";
constexpr const char* audioSampleRateKey = "samplerate";
} // namespace

const GQuark Encoder::errorDomain = Glib::Quark("EncoderErrorDomain");
const int Encoder::sameAsSource = -1;

std::shared_ptr<Encoder> Encoder::createEncoder(const std::string& type)
{
    if (type == Mp4Encoder::type)
    {
        return std::make_shared<Mp4Encoder>();
    }

    if (type == WebmEncoder::type)
    {
        return std::make_shared<WebmEncoder>();
    }

    throw InvalidTypeException();
}

Encoder::Encoder()
    : m_videoWidth(sameAsSource), m_videoHeight(sameAsSource), m_frameRateNumerator(sameAsSource),
      m_frameRateDenominator(1), m_audioChannels(sameAsSource), m_audioSampleRate(sameAsSource)
{
    m_encodeBin = Gst::EncodeBin::create();
    m_fileSink = Gst::FileSink::create();

    if (!m_encodeBin || !m_fileSink)
    {
        throw UnrecoverableError();
    }
}

Encoder::~Encoder()
{
    cleanupEncoder();
}

void Encoder::setOutputFile(const Glib::ustring& file) noexcept
{
    m_outputFile = file;
}

void Encoder::setVideoDimensions(int width, int height) noexcept
{
    m_videoWidth = (width > 0) ? width : sameAsSource;
    m_videoHeight = (height > 0) ? height : sameAsSource;
}

void Encoder::setVideoFrameRate(int numerator, int denominator) noexcept
{
    m_frameRateNumerator = (numerator > 0) ? numerator : sameAsSource;
    m_frameRateDenominator = (denominator > 0) ? denominator : 1;
}

void Encoder::setAudioChannels(int n) noexcept
{
    m_audioChannels = (n > 0) ? n : sameAsSource;
}

void Encoder::setAudioSampleRate(int rate) noexcept
{
    m_audioSampleRate = (rate > 0) ? rate : sameAsSource;
}

void Encoder::onPlayerPrerolled(Player& player)
{
    // Add encoder elements to pipeline.
    player.getPipeline()->add(m_encodeBin)->add(m_fileSink);
    m_encodeBin->link(m_fileSink);

    m_fileSink->property_location() = m_outputFile;
    m_encodeBin->property_profile() = createEncodingProfile();

    if (!m_encodeBin->sync_state_with_parent() || !m_fileSink->sync_state_with_parent())
    {
        throw InvalidStateException();
    }

    // Connect encoder to player.
    player.forEachConnector([this](Connector& connector) {
        Glib::RefPtr<Gst::Pad> sinkPad;
        if ((connector.getStreamType() & GST_STREAM_TYPE_VIDEO) != 0)
        {
            sinkPad = this->m_encodeBin->get_request_pad("video_%u");
        }
        else if ((connector.getStreamType() & GST_STREAM_TYPE_AUDIO) != 0)
        {
            sinkPad = this->m_encodeBin->get_request_pad("audio_%u");
        }
        else
        {
            return;
        }

        connector.connect(sinkPad);
    });

    // Configure codecs.
    auto it = m_encodeBin->iterate_elements();
    while (it.next() == Gst::ITERATOR_OK)
    {
        auto factory = it->get_factory();
        if (factory)
        {
            if ((bool)g_type_is_a(factory->get_element_type(), GST_TYPE_VIDEO_ENCODER))
            {
                try
                {
                    onConfigureVideoEncoder(*it);
                }
                catch (const std::exception& e)
                {
                    player.getPipeline()->get_bus()->post(Gst::MessageWarning::create(
                        *it,
                        Glib::Error(errorDomain, static_cast<int>(ErrorCode::cannotConfigureVideoEncoder),
                                    "cannot configure video encoder"),
                        e.what()));
                }
            }
            else if ((bool)g_type_is_a(factory->get_element_type(), GST_TYPE_AUDIO_ENCODER))
            {
                try
                {
                    onConfigureAudioEncoder(*it);
                }
                catch (const std::exception& e)
                {
                    player.getPipeline()->get_bus()->post(Gst::MessageWarning::create(
                        *it,
                        Glib::Error(errorDomain, static_cast<int>(ErrorCode::cannotConfigureAudioEncoder),
                                    "cannot configure audio encoder"),
                        e.what()));
                }
            }
        }
    }
}

void Encoder::onPlayerPlaying(Player& /*player*/) noexcept
{
    // Empty method.
}

void Encoder::onPlayerStopped(Player& /*player*/, bool /*isInterrupted*/) noexcept
{
    cleanupEncoder();
}

void Encoder::onPipelineIssue(Player& /*player*/, bool /*isFatalError*/, const Glib::Error& /*error*/,
                              const std::string& /*debugMessage*/) noexcept
{
    // Empty method.
}

Json Encoder::serialize() const
{
    Json obj = Json::object();
    if (!m_outputFile.empty())
    {
        obj[outputFileKey] = m_outputFile.c_str();
    }

    if (m_videoWidth != sameAsSource)
    {
        obj[videoWidthKey] = m_videoWidth;
    }

    if (m_videoHeight != sameAsSource)
    {
        obj[videoHeightKey] = m_videoHeight;
    }

    if (m_frameRateNumerator != sameAsSource)
    {
        if (m_frameRateDenominator != 1)
        {
            obj[frameRateKey] = {m_frameRateNumerator, m_frameRateDenominator};
        }
        else
        {
            obj[frameRateKey] = m_frameRateNumerator;
        }
    }

    if (m_audioChannels != sameAsSource)
    {
        obj[audioChannelsKey] = m_audioChannels;
    }

    if (m_audioSampleRate != sameAsSource)
    {
        obj[audioSampleRateKey] = m_audioSampleRate;
    }

    return obj;
}

void Encoder::unserialize(const Json& in)
{
    if (in.contains(outputFileKey))
    {
        setOutputFile(in.at(outputFileKey).get<std::string>());
    }
    else
    {
        setOutputFile("");
    }

    int width = sameAsSource;
    int height = sameAsSource;
    if (in.contains(videoWidthKey))
    {
        width = in.at(videoWidthKey).get<int>();
    }
    if (in.contains(videoHeightKey))
    {
        height = in.at(videoHeightKey).get<int>();
    }
    setVideoDimensions(width, height);

    int numerator = sameAsSource;
    int denominator = 1;
    if (in.contains(frameRateKey))
    {
        const Json& entry = in.at(frameRateKey);
        if (entry.is_array())
        {
            numerator = entry[0].get<int>();
            denominator = entry[1].get<int>();
        }
        else
        {
            numerator = entry.get<int>();
        }
    }
    setVideoFrameRate(numerator, denominator);

    int channels = sameAsSource;
    if (in.contains(audioChannelsKey))
    {
        channels = in.at(audioChannelsKey).get<int>();
    }
    setAudioChannels(channels);

    int sampleRate = sameAsSource;
    if (in.contains(audioSampleRateKey))
    {
        sampleRate = in.at(audioSampleRateKey).get<int>();
    }
    setAudioSampleRate(sampleRate);
}

void Encoder::onConfigureVideoEncoder(const Glib::RefPtr<Gst::Element>& /*element*/)
{
    // Empty default implementation.
}

void Encoder::onConfigureAudioEncoder(const Glib::RefPtr<Gst::Element>& /*element*/)
{
    // Empty default implementation.
}

Glib::RefPtr<Gst::Caps> Encoder::getVideoCaps() const noexcept
{
    Gst::Structure data("video/x-raw");
    if (m_videoWidth != sameAsSource)
    {
        data.set_field("width", m_videoWidth);
    }

    if (m_videoHeight != sameAsSource)
    {
        data.set_field("height", m_videoHeight);
    }

    if (m_frameRateNumerator != sameAsSource)
    {
        data.set_field("framerate", Gst::Fraction(m_frameRateNumerator, m_frameRateDenominator));
    }

    return Gst::Caps::create(data);
}

Glib::RefPtr<Gst::Caps> Encoder::getAudioCaps() const noexcept
{
    Gst::Structure data("audio/x-raw");
    if (m_audioChannels != sameAsSource)
    {
        data.set_field("channels", m_audioChannels);
    }

    if (m_audioSampleRate != sameAsSource)
    {
        data.set_field("rate", m_audioSampleRate);
    }

    return Gst::Caps::create(data);
}

Glib::RefPtr<Gst::EncodingProfile> Encoder::createEncodingProfile() const
{
    auto caps = Gst::Caps::create_from_string(getContainerType());
    GstEncodingContainerProfile* profile = gst_encoding_container_profile_new(nullptr, nullptr, caps->gobj(), nullptr);

    caps = Gst::Caps::create_from_string(getVideoType());
    if (!(bool)gst_encoding_container_profile_add_profile(
            profile, reinterpret_cast<GstEncodingProfile*>( // NOLINT
                         gst_encoding_video_profile_new(caps->gobj(), nullptr, getVideoCaps()->gobj(), 0))))
    {
        throw CannotCreateEncodingProfileException();
    }

    caps = Gst::Caps::create_from_string(getAudioType());
    if (!(bool)gst_encoding_container_profile_add_profile(
            profile, reinterpret_cast<GstEncodingProfile*>( // NOLINT
                         gst_encoding_audio_profile_new(caps->gobj(), nullptr, getAudioCaps()->gobj(), 0))))
    {
        throw CannotCreateEncodingProfileException();
    }

    return Glib::wrap(reinterpret_cast<GstEncodingProfile*>(profile)); // NOLINT
}

void Encoder::cleanupEncoder() noexcept
{
    m_encodeBin->set_state(Gst::STATE_NULL);
    m_fileSink->set_state(Gst::STATE_NULL);

    auto parent = Glib::RefPtr<Gst::Bin>::cast_static(m_encodeBin->get_parent());
    if (parent)
    {
        parent->remove(m_encodeBin);
    }

    parent = Glib::RefPtr<Gst::Bin>::cast_static(m_fileSink->get_parent());
    if (parent)
    {
        parent->remove(m_fileSink);
    }

    auto it = m_encodeBin->iterate_sink_pads();
    while (it.next() == Gst::ITERATOR_OK)
    {
        m_encodeBin->release_request_pad(*it);
    }
}
