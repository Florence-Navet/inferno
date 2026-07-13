cmake_minimum_required(VERSION 3.20)
project(InfernoTransport LANGUAGES CXX)

# ── OpenSSL for TLS communication ────────────────────────────────────────────
find_package(OpenSSL REQUIRED)

# ── Library ───────────────────────────────────────────────────────────────────
# STATIC = compiled once into a .a archive, then linked into agent and server.
# No Dockerfile, no executable — this is purely a library.
add_library(transport_lib STATIC
    src/frame_transport.cpp
    src/env_helper.cpp
    src/logger.cpp
    src/dispatcher/dispatcher.cpp
    src/codec/convert_endian.cpp
    src/codec/protocol_parser.cpp
    src/codec/protocol_serializer.cpp
    src/codec/metrics_serializer.cpp
    src/codec/metrics_parser.cpp
    src/codec/protocol_helper.cpp
    src/socket/linux_socket.cpp
    src/socket/socket_factory.cpp
    src/socket/tls_socket.cpp
    src/socket/tls_socket_factory.cpp
    src/poller/poller.cpp
)

target_link_libraries(transport_lib PUBLIC OpenSSL::SSL OpenSSL::Crypto)

# PUBLIC = these include dirs are inherited by any target that links common_lib.
# This is what lets agent and server write #include "ISocket.hpp" without paths.
target_include_directories(transport_lib PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_compile_options(transport_lib PRIVATE
    -Wall -Wextra -Wpedantic
)

# ── Tests ─────────────────────────────────────────────────────────────────────
if(CMAKE_PROJECT_NAME STREQUAL "InfernoTransport")
    enable_testing()
    find_package(GTest REQUIRED)

    add_subdirectory(
        ${CMAKE_CURRENT_SOURCE_DIR}/../test_support
        ${CMAKE_CURRENT_BINARY_DIR}/test_support
    )

    set(PARSER_TESTS
        tests/codec/protocol_parser_test.cpp
    )

    set(SERIALIZER_TESTS
        tests/codec/protocol_serializer_test.cpp
    )

    set(PARSER_SERIALIZER_TESTS
        tests/codec/protocol_roundtrip_test.cpp
        tests/codec/round_trip_metrics_test.cpp
        tests/codec/round_trip_serialization_test.cpp
    )

    set(SOCKET_TESTS
        tests/socket/socket_integration_test.cpp
        tests/socket/tls_socket_integration_test.cpp
        )
        
        add_executable(transport_tests
        tests/convert_endian_test.cpp
        tests/frame_transport_test.cpp
        ${PARSER_SERIALIZER_TESTS}
        ${PARSER_TESTS}
        ${SERIALIZER_TESTS}
        ${SOCKET_TESTS}
    )

    target_include_directories(transport_tests PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/tests
    )

    target_link_libraries(transport_tests PRIVATE
        transport_lib
        test_support
        GTest::gtest
        GTest::gmock
        GTest::gtest_main
    )

    target_compile_options(transport_tests PRIVATE
        -Wall -Wextra -Wpedantic
    )
    
    add_test(NAME transport_tests COMMAND transport_tests)
    set_tests_properties(transport_tests PROPERTIES
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}/..
    )
    # include(GoogleTest)
    # gtest_discover_tests(common_tests DISCOVERY_MODE PRE_TEST)

endif()