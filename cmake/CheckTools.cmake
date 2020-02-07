###############################################################################
# Configure clang-format and clang-tidy with cmake.
#
# Copyright (C) 2020, Loïc Le Page
#
# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program.  If not, see <http://www.gnu.org/licenses/>.
#
###############################################################################

option(USE_CODE_FORMAT "Use code auto-formatting on build" ON)
option(USE_CODE_TIDY "Use code static analysis on build" ON)

function(configure_clang_format)
    find_program(CLANG_FORMAT_EXE
                 NAMES clang-format-10 clang-format
                 DOC "Path to clang-format executable")
    if(CLANG_FORMAT_EXE)
        message(STATUS "Using clang-format from ${CLANG_FORMAT_EXE}")
        set(CLANG_FORMAT_CLI "${CLANG_FORMAT_EXE}" --Werror -style=file -i PARENT_SCOPE)
    else()
        message(WARNING "clang-format not found, please install tool and relaunch configuration")
    endif()
endfunction()

if(USE_CODE_FORMAT AND NOT CLANG_FORMAT_CLI)
    configure_clang_format()
endif()

function(configure_clang_tidy)
    find_program(CLANG_TIDY_EXE
                 NAMES clang-tidy-10 clang-tidy
                 DOC "Path to clang-tidy executable")
    if(CLANG_TIDY_EXE)
        message(STATUS "Using clang-tidy from ${CLANG_TIDY_EXE}")
        file(RELATIVE_PATH PATH_REGEX "${CMAKE_CURRENT_BINARY_DIR}" "${CMAKE_CURRENT_SOURCE_DIR}")
        set(PATH_REGEX "${CMAKE_CURRENT_BINARY_DIR}/${PATH_REGEX}(include|src)/.*")
        set(CLANG_TIDY_CLI "${CLANG_TIDY_EXE}" --header-filter="${PATH_REGEX}" PARENT_SCOPE)
    else()
        message(WARNING "clang-tidy not found, please install tool and relaunch configuration")
    endif()
endfunction()

if(USE_CODE_TIDY AND NOT CLANG_TIDY_CLI)
    configure_clang_tidy()
endif()

function(target_configure_cxx_checks TARGET_NAME)
    set_target_properties(${TARGET_NAME} PROPERTIES
                          CXX_EXTENSIONS OFF)
    if(CLANG_FORMAT_CLI)
        get_target_property(TARGET_SOURCES ${TARGET_NAME} SOURCES)
        if(TARGET_SOURCES)
            add_custom_command(TARGET ${TARGET_NAME} PRE_BUILD
                               COMMAND ${CLANG_FORMAT_CLI} ${TARGET_SOURCES}
                               WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}")
        endif()
    endif()
    if(CLANG_TIDY_CLI)
        set_target_properties(${TARGET_NAME} PROPERTIES CXX_CLANG_TIDY "${CLANG_TIDY_CLI}")
    endif()
    if(MSVC)
        target_compile_options(${TARGET_NAME} PRIVATE /W4 /WX)
    else()
        target_compile_options(${TARGET_NAME} PRIVATE -Wall -Wextra -Werror)
    endif()
endfunction()
