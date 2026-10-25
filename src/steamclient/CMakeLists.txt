if(WIN32)
    include(FetchContent)
    set(OPUS_BUILD_SHARED_LIBRARY OFF CACHE BOOL "" FORCE)
    set(OPUS_BUILD_TESTING OFF CACHE BOOL "" FORCE)
    set(OPUS_BUILD_PROGRAMS OFF CACHE BOOL "" FORCE)
    set(OPUS_INSTALL_PKG_CONFIG_MODULE OFF CACHE BOOL "" FORCE)
    set(OPUS_INSTALL_CMAKE_CONFIG_MODULE OFF CACHE BOOL "" FORCE)
    set(OPUS_STATIC_RUNTIME ON CACHE BOOL "" FORCE)
    FetchContent_Declare(opus
        GIT_REPOSITORY https://github.com/xiph/opus.git
        GIT_TAG v1.5.2
        GIT_SHALLOW TRUE
    )
    FetchContent_MakeAvailable(opus)

    add_library(steamclient SHARED
        dllmain.cpp
        identity.cpp
        vellum_config.cpp
        steam_ifaces.cpp
        steam_query.cpp
        steam_http.cpp
        steam_voice.cpp
        steamclient.def
    )
    target_link_libraries(steamclient PRIVATE advapi32 ws2_32 wininet winhttp winmm opus)
    target_compile_definitions(steamclient PRIVATE _CRT_SECURE_NO_WARNINGS)
else()
    add_library(steamclient SHARED
        identity.cpp
        vellum_config.cpp
        steam_ifaces.cpp
        steam_query.cpp
        steam_http.cpp
        steam_voice.cpp
    )
    target_compile_options(steamclient PRIVATE -fvisibility=hidden)
    target_link_options(steamclient PRIVATE -static-libstdc++ -static-libgcc)
    target_link_libraries(steamclient PRIVATE pthread dl opus asound)
endif()

# Exactly one auth implementation is compiled and linked.
if(VELLUM_AUTH_REVEMU2013)
    set(VELLUM_AUTH_SOURCES revemu2013.cpp)
    target_compile_definitions(steamclient PRIVATE VELLUM_AUTH_REVEMU2013)
    message(STATUS "loader auth: RevEmu 2013 only")
else()
    set(VELLUM_AUTH_SOURCES
        ${AUTHUM_DIR}/src/client/ed25519.c
        ${AUTHUM_DIR}/lib/monocypher/monocypher.c
        ${AUTHUM_DIR}/lib/monocypher/monocypher-ed25519.c
    )
    if(WIN32)
        target_link_libraries(steamclient PRIVATE bcrypt)
    endif()
    message(STATUS "loader auth: Vellum v2 only")
endif()
target_sources(steamclient PRIVATE ${VELLUM_AUTH_SOURCES})

target_include_directories(steamclient PRIVATE
    ${CMAKE_SOURCE_DIR}/src/common
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${AUTHUM_DIR}/include
    ${AUTHUM_DIR}/lib/monocypher
)

if (MSVC)
    target_compile_options(steamclient PRIVATE /utf-8)
endif()

if(VELLUM_NO_SERVER_BROWSER)
    target_compile_definitions(steamclient PRIVATE VELLUM_NO_SERVER_BROWSER)
    message(STATUS "loader: Internet master query compiled out")
endif()

set_target_properties(steamclient PROPERTIES
    OUTPUT_NAME steamclient
    PREFIX ""
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}"
    LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}"
    ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}"
)

# Explicit smoke-test target; excluded from normal game builds.
add_executable(auth_ticket_smoke EXCLUDE_FROM_ALL
    ${CMAKE_SOURCE_DIR}/tools/auth_ticket_smoke.cpp
    identity.cpp
    ${VELLUM_AUTH_SOURCES}
)
target_include_directories(auth_ticket_smoke PRIVATE $<TARGET_PROPERTY:steamclient,INCLUDE_DIRECTORIES>)
target_compile_definitions(auth_ticket_smoke PRIVATE $<TARGET_PROPERTY:steamclient,COMPILE_DEFINITIONS>)
if(WIN32)
    target_link_libraries(auth_ticket_smoke PRIVATE advapi32)
    if(NOT VELLUM_AUTH_REVEMU2013)
        target_link_libraries(auth_ticket_smoke PRIVATE bcrypt)
    endif()
endif()
if(MSVC)
    target_compile_options(auth_ticket_smoke PRIVATE /utf-8 /UNDEBUG)
else()
    target_compile_options(auth_ticket_smoke PRIVATE -UNDEBUG)
endif()
set_target_properties(auth_ticket_smoke PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}"
)
