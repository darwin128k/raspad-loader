# One wrapper per game.
#
# The same loader source is built into a separate executable for every game
# it should start:
#
#   cstrike.exe  - Counter-Strike 1.6
#   svencoop.exe - Sven Co-op
#
# Each wrapper knows its own name, game directory, Steam AppId, icon and
# instance mutex.  None of them accepts a "-game" from the caller: the game
# is fixed at build time, everything else is passed through to the engine.
set(VELLUM_NAME "cstrike" CACHE STRING "Wrapper edition: cstrike, svencoop, ...")
set(VELLUM_GAME "" CACHE STRING "Game directory the wrapper starts (default: same as the edition name)")
set(VELLUM_APPID "" CACHE STRING "Steam AppId of the game (default: 10)")

if(VELLUM_GAME STREQUAL "")
    set(VELLUM_GAME "${VELLUM_NAME}")
endif()
if(VELLUM_APPID STREQUAL "")
    set(VELLUM_APPID "10")
endif()

message(STATUS "loader: wrapper '${VELLUM_NAME}' starts -game ${VELLUM_GAME} (AppId ${VELLUM_APPID})")

if(WIN32)
    add_executable(${VELLUM_NAME} WIN32 main.cpp)

    # The icon lives in res/<edition>.rc; editions without one fall back to
    # the cstrike icon.
    set(VELLUM_RC "${CMAKE_SOURCE_DIR}/res/${VELLUM_NAME}.rc")
    if(NOT EXISTS "${VELLUM_RC}")
        set(VELLUM_RC "${CMAKE_SOURCE_DIR}/res/cstrike.rc")
    endif()

    set(VELLUM_RES "${CMAKE_CURRENT_BINARY_DIR}/${VELLUM_NAME}.res")
    add_custom_command(
        OUTPUT "${VELLUM_RES}"
        COMMAND "${CMAKE_RC_COMPILER}" /nologo /i "${CMAKE_SOURCE_DIR}/res" /fo "${VELLUM_RES}" "${VELLUM_RC}"
        DEPENDS "${VELLUM_RC}"
        COMMENT "Embedding the wrapper icon"
        VERBATIM
    )
    target_sources(${VELLUM_NAME} PRIVATE "${VELLUM_RES}")
    set_source_files_properties("${VELLUM_RES}" PROPERTIES EXTERNAL_OBJECT TRUE GENERATED TRUE)
    target_link_libraries(${VELLUM_NAME} PRIVATE advapi32 shell32 user32)
    target_compile_definitions(${VELLUM_NAME} PRIVATE _CRT_SECURE_NO_WARNINGS)
else()
    add_executable(${VELLUM_NAME} main_linux.cpp)
endif()

target_compile_definitions(${VELLUM_NAME} PRIVATE
    VELLUM_NAME="${VELLUM_NAME}"
    VELLUM_GAME="${VELLUM_GAME}"
    VELLUM_APPID="${VELLUM_APPID}")

if(VELLUM_STANDALONE)
    if(HL_DIR STREQUAL "")
        message(FATAL_ERROR "VELLUM_STANDALONE=ON requires -DHL_DIR=/path/to/raspad-hl")
    endif()
    get_filename_component(HL_DIR "${HL_DIR}" ABSOLUTE)
    if(NOT EXISTS "${HL_DIR}/src/launcher.cpp")
        message(FATAL_ERROR "HL_DIR=${HL_DIR} does not contain src/launcher.cpp")
    endif()
    target_sources(${VELLUM_NAME} PRIVATE "${HL_DIR}/src/launcher.cpp")
    target_include_directories(${VELLUM_NAME} PRIVATE "${HL_DIR}/src")
    target_compile_definitions(${VELLUM_NAME} PRIVATE REVLOADER_STANDALONE)
    message(STATUS "vellum standalone: using hl sources from ${HL_DIR}")
    if(NOT WIN32)
        target_link_libraries(${VELLUM_NAME} PRIVATE dl)
    endif()
    if(HL_METAHOOK)
        if(METAHOOK_DIR STREQUAL "")
            set(METAHOOK_DIR "${HL_DIR}/../metahook")
        endif()
        include("${HL_DIR}/cmake/MetaHookEmbed.cmake")
        hl_embed_metahook(${VELLUM_NAME} "${METAHOOK_DIR}")
    endif()
elseif(NOT WIN32)
    target_link_options(${VELLUM_NAME} PRIVATE -static -static-libstdc++ -static-libgcc)
endif()

if (MSVC)
    target_compile_options(${VELLUM_NAME} PRIVATE /utf-8)
endif()
if(WIN32)
    set(_vellum_out ${VELLUM_NAME})
else()
    set(_vellum_out ${VELLUM_NAME}_linux)
endif()
set_target_properties(${VELLUM_NAME} PROPERTIES OUTPUT_NAME "${_vellum_out}" RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}")

# Explicit regression check for argument quoting; excluded from game builds.
add_executable(loader_args_smoke EXCLUDE_FROM_ALL ${CMAKE_SOURCE_DIR}/tools/loader_args_smoke.cpp)
target_include_directories(loader_args_smoke PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
if(WIN32)
    target_link_libraries(loader_args_smoke PRIVATE shell32)
endif()
if(MSVC)
    target_compile_options(loader_args_smoke PRIVATE /UNDEBUG)
else()
    target_compile_options(loader_args_smoke PRIVATE -UNDEBUG)
endif()
set_target_properties(loader_args_smoke PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}")
