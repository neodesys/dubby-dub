
#include "Encoder.h"
#include "exceptions.h"
#include <cassert>

const GQuark Encoder::errorDomain = Glib::Quark("EncoderErrorDomain");
const int Encoder::sameAsSource = -1;

Encoder::Encoder()
    : m_videoWidth(sameAsSource), m_videoHeight(sameAsSource), m_framerateNumerator(sameAsSource),
      m_framerateDenominator(1), m_audioChannels(sameAsSource), m_audioSampleRate(sameAsSource)
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
    cleanupAfterEncoding();
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

void Encoder::setVideoFramerate(int numerator, int denominator) noexcept
{
    m_framerateNumerator = (numerator > 0) ? numerator : sameAsSource;
    m_framerateDenominator = (denominator > 0) ? denominator : 1;
}

void Encoder::setAudioChannels(int n) noexcept
{
    m_audioChannels = (n > 0) ? n : sameAsSource;
}

void Encoder::setAudioSampleRate(int rate) noexcept
{
    m_audioSampleRate = (rate > 0) ? rate : sameAsSource;
}

bool Encoder::prepareEncoding(const Glib::RefPtr<Gst::Pipeline>& pipeline) noexcept
{
    try
    {
        pipeline->add(m_encodeBin)->add(m_fileSink);
        m_encodeBin->link(m_fileSink);

        m_fileSink->property_location() = m_outputFile;
        m_encodeBin->property_profile() = createEncodingProfile();

        if (!m_encodeBin->sync_state_with_parent() || !m_fileSink->sync_state_with_parent())
        {
            throw InvalidStateException();
        }

        return true;
    }
    catch (const std::exception& e)
    {
        cleanupAfterEncoding();
        pipeline->get_bus()->post(
            Gst::MessageError::create(m_encodeBin,
                                      Glib::Error(errorDomain, static_cast<int>(ErrorCode::cannotAddToPipeline),
                                                  "cannot add encoder to pipeline"),
                                      e.what()));
        return false;
    }
}

bool Encoder::linkPad(const Glib::RefPtr<Gst::Pad>& pad, bool isVideoPad) noexcept
{
    auto pipeline = Glib::RefPtr<Gst::Pipeline>::cast_static(m_encodeBin->get_parent());
    assert(pipeline); // NOLINT

    Glib::RefPtr<Gst::Pad> sinkPad;
    if (isVideoPad)
    {
        sinkPad = m_encodeBin->get_request_pad("video_%u");
    }
    else
    {
        sinkPad = m_encodeBin->get_request_pad("audio_%u");
    }

    if (!sinkPad || (pad->link(sinkPad) != Gst::PAD_LINK_OK))
    {
        const char* cause = "encoder sink pad request failed";
        if (sinkPad)
        {
            cause = "link to encoder sink pad failed";
            m_encodeBin->release_request_pad(sinkPad);
        }

        pipeline->get_bus()->post(
            Gst::MessageError::create(m_encodeBin,
                                      Glib::Error(errorDomain, static_cast<int>(ErrorCode::cannotLinkStream),
                                                  "cannot link raw stream to encoder"),
                                      cause));
        return false;
    }

    return true;
}

void Encoder::configureEncoders() noexcept
{
    auto pipeline = Glib::RefPtr<Gst::Pipeline>::cast_static(m_encodeBin->get_parent());
    assert(pipeline); // NOLINT

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
                    pipeline->get_bus()->post(Gst::MessageWarning::create(
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
                    pipeline->get_bus()->post(Gst::MessageWarning::create(
                        *it,
                        Glib::Error(errorDomain, static_cast<int>(ErrorCode::cannotConfigureAudioEncoder),
                                    "cannot configure audio encoder"),
                        e.what()));
                }
            }
        }
    }
}

void Encoder::cleanupAfterEncoding() noexcept
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

void Encoder::onConfigureVideoEncoder(const Glib::RefPtr<Gst::Element>& /*element*/)
{
    // Empty default implementation
}

void Encoder::onConfigureAudioEncoder(const Glib::RefPtr<Gst::Element>& /*element*/)
{
    // Empty default implementation
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

    if (m_framerateNumerator != sameAsSource)
    {
        data.set_field("framerate", Gst::Fraction(m_framerateNumerator, m_framerateDenominator));
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
    gst_encoding_container_profile_add_profile(
        profile, reinterpret_cast<GstEncodingProfile*>( // NOLINT
                     gst_encoding_video_profile_new(caps->gobj(), nullptr, getVideoCaps()->gobj(), 0)));

    caps = Gst::Caps::create_from_string(getAudioType());
    gst_encoding_container_profile_add_profile(
        profile, reinterpret_cast<GstEncodingProfile*>( // NOLINT
                     gst_encoding_audio_profile_new(caps->gobj(), nullptr, getAudioCaps()->gobj(), 0)));

    return Glib::wrap(reinterpret_cast<GstEncodingProfile*>(profile)); // NOLINT
}
