include(FindPkgConfig)
pkg_check_modules(GSTMM REQUIRED gstreamermm-1.0)

include(FetchContent)
FetchContent_Declare(json
                     GIT_REPOSITORY https://github.com/ArthurSonzogni/nlohmann_json_cmake_fetchcontent
                     GIT_TAG v3.7.3)
FetchContent_MakeAvailable(json)

add_executable(${PROJECT_NAME} main.cpp exceptions.h ISerializable.h
                               Transcoder.h Transcoder.cpp
                               player/IPlayerListener.h
                               player/Player.h player/Player.cpp
                               player/Connector.h player/Connector.cpp
                               encoders/Encoder.h encoders/Encoder.cpp
                               encoders/WebmEncoder.h encoders/WebmEncoder.cpp
                               encoders/Mp4Encoder.h encoders/Mp4Encoder.cpp)
target_configure_cxx_checks(${PROJECT_NAME})

target_include_directories(${PROJECT_NAME} PRIVATE "${GSTMM_INCLUDE_DIRS}")
target_link_libraries(${PROJECT_NAME} PRIVATE "${GSTMM_LIBRARIES}" nlohmann_json::nlohmann_json)
