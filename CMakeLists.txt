cmake_minimum_required(VERSION 3.16)
project(raspad_loader LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")

if(WIN32)
    enable_language(RC)
elseif(UNIX AND NOT APPLE)
    # GoldSrc client is 32-bit. Match steam_api.so / hw.so and raspad-hl.
    message(STATUS "loader: targeting 32-bit Linux (hw.so)")
    add_compile_options(-m32 -Wno-format-truncation)
    add_link_options(-m32)
else()
    message(FATAL_ERROR "Raspad Loader supports Windows and Linux")
endif()

option(VELLUM_STANDALONE "Run GoldSrc in this process (compile hl launcher sources from raspad-hl) instead of spawning hl" OFF)
option(REVLOADER_STANDALONE "Deprecated alias of VELLUM_STANDALONE" OFF)
option(HL_METAHHOOK "Embed MetaHookSv in the standalone launcher" OFF)
option(VELLUM_AUTH_REVEMU2013 "Use RevEmu 2013 tickets instead of Vellum v2" OFF)
option(VELLUM_NO_SERVER_BROWSER "Compile out Internet master queries (favorites and LAN stay)" OFF)
set(HL_DIR "" CACHE PATH "Path to raspad-hl (required when VELLUM_STANDALONE=ON)")
set(METAHOOK_DIR "" CACHE PATH "Path to MetaHookSv (default: HL_DIR/../metahook)")

if(REVLOADER_STANDALONE)
    set(VELLUM_STANDALONE ON)
endif()

set(AUTHUM_DIR "${CMAKE_SOURCE_DIR}/lib/authum")
if(NOT VELLUM_AUTH_REVEMU2013 AND NOT EXISTS "${AUTHUM_DIR}/lib/monocypher/monocypher.c")
    message(FATAL_ERROR "lib/authum missing - run: git submodule update --init lib/authum")
endif()

if(HL_METAHHOOK AND NOT WIN32)
    message(FATAL_ERROR "HL_METAHHOOK is Windows-only")
endif()

add_subdirectory(src/loader)
add_subdirectory(src/steamclient)
