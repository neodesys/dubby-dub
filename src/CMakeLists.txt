include(FindPkgConfig)
pkg_check_modules(GSTMM REQUIRED gstreamermm-1.0)

add_executable(${PROJECT_NAME} main.cpp)
target_configure_cxx_checks(${PROJECT_NAME})

target_include_directories(${PROJECT_NAME} PRIVATE "${GSTMM_INCLUDE_DIRS}")
target_link_libraries(${PROJECT_NAME} PRIVATE "${GSTMM_LIBRARIES}")
