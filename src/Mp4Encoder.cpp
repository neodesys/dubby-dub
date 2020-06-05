#include "Mp4Encoder.h"

const char* Mp4Encoder::getContainerType() const noexcept
{
    return "video/mp4";
}

const char* Mp4Encoder::getVideoType() const noexcept
{
    return "video/x-h265";
}

const char* Mp4Encoder::getAudioType() const noexcept
{
    return "audio/mpeg";
}
