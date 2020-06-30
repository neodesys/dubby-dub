include(FindPkgConfig)
pkg_check_modules(GSTMM REQUIRED gstreamermm-1.0)

include(FetchContent)
FetchContent_Declare(json
                     GIT_REPOSITORY https://github.com/ArthurSonzogni/nlohmann_json_cmake_fetchcontent
                     GIT_TAG v3.7.3)
FetchContent_MakeAvailable(json)

file(WRITE "${CMAKE_CURRENT_BINARY_DIR}/version.cpp" "const char* dubbyDubVersion = \"${PROJECT_VERSION}\"; // NOLINT")
add_executable(${PROJECT_NAME} "${CMAKE_CURRENT_BINARY_DIR}/version.cpp"
                               main.cpp exceptions.h ISerializable.h
                               Transcoder.h Transcoder.cpp
                               player/IPlayerListener.h
                               player/Player.h player/Player.cpp
                               player/Connector.h player/Connector.cpp
                               encoders/Encoder.h encoders/Encoder.cpp
                               encoders/WebmEncoder.h encoders/WebmEncoder.cpp
                               encoders/Mp4Encoder.h encoders/Mp4Encoder.cpp
                               encoders/OggEncoder.h encoders/OggEncoder.cpp
                               encoders/MkvEncoder.h encoders/MkvEncoder.cpp
                               codecs/Codec.h codecs/Codec.cpp
                               codecs/BitrateCodec.h codecs/BitrateCodec.cpp
                               codecs/BitrateOrQualityCodec.h codecs/BitrateOrQualityCodec.cpp
                               codecs/video/H264Codec.h codecs/video/H264Codec.cpp
                               codecs/video/H265Codec.h codecs/video/H265Codec.cpp
                               codecs/video/TheoraCodec.h codecs/video/TheoraCodec.cpp
                               codecs/video/Vp8Codec.h codecs/video/Vp8Codec.cpp
                               codecs/video/Vp9Codec.h codecs/video/Vp9Codec.cpp
                               codecs/audio/AacCodec.h codecs/audio/AacCodec.cpp
                               codecs/audio/Mp3Codec.h codecs/audio/Mp3Codec.cpp
                               codecs/audio/OpusCodec.h codecs/audio/OpusCodec.cpp
                               codecs/audio/VorbisCodec.h codecs/audio/VorbisCodec.cpp)
target_configure_cxx_checks(${PROJECT_NAME})

target_include_directories(${PROJECT_NAME} PRIVATE "${GSTMM_INCLUDE_DIRS}")
target_link_libraries(${PROJECT_NAME} PRIVATE "${GSTMM_LIBRARIES}" nlohmann_json::nlohmann_json)
