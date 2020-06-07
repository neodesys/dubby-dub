#include "WebmEncoder.h"

const char* WebmEncoder::getContainerType() const noexcept
{
    return "video/webm";
}

const char* WebmEncoder::getVideoType() const noexcept
{
    return "video/x-vp9";
}

const char* WebmEncoder::getAudioType() const noexcept
{
    return "audio/x-vorbis";
}
