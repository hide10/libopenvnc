cmake_minimum_required(VERSION 3.15)
project(libopenvnc
    VERSION 0.1.0
    LANGUAGES C
    DESCRIPTION "A cross-platform VNC (RFB) protocol library"
)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

# Options
option(OVNC_BUILD_TESTS "Build tests" ON)
option(OVNC_BUILD_EXAMPLES "Build examples" ON)

# Generate version header
configure_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/cmake/ovnc_version.h.in
    ${CMAKE_CURRENT_BINARY_DIR}/include/ovnc/ovnc_version.h
    @ONLY
)

# Library sources
set(OVNC_SOURCES
    src/common/ovnc_error.c
    src/common/ovnc_pixel.c
    src/transport/ovnc_transport.c
    src/protocol/ovnc_handshake.c
    src/protocol/ovnc_messages.c
    src/protocol/ovnc_encoding.c
    src/protocol/ovnc_enc_raw.c
    src/client/ovnc_client.c
    src/server/ovnc_server.c
)

# Platform-specific sources
if(WIN32)
    list(APPEND OVNC_SOURCES src/platform/ovnc_socket_win32.c)
else()
    list(APPEND OVNC_SOURCES src/platform/ovnc_socket_posix.c)
endif()

# Static library
add_library(ovnc STATIC ${OVNC_SOURCES})

target_include_directories(ovnc
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}/include>
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/src
)

# Platform libraries
if(WIN32)
    target_link_libraries(ovnc PRIVATE ws2_32)
endif()

# Compiler warnings
if(CMAKE_C_COMPILER_ID MATCHES "GNU|Clang")
    target_compile_options(ovnc PRIVATE -Wall -Wextra -Wpedantic)
elseif(MSVC)
    target_compile_options(ovnc PRIVATE /W4)
endif()

# Tests
if(OVNC_BUILD_TESTS)
    add_subdirectory(tests)
endif()

# Examples
if(OVNC_BUILD_EXAMPLES)
    add_subdirectory(examples)
endif()
