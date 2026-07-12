cmake_minimum_required(VERSION 3.20)
project(InfernoCommon LANGUAGES CXX)

# ── OpenSSL for TLS communication ────────────────────────────────────────────
find_package(OpenSSL REQUIRED)

# ── Library ───────────────────────────────────────────────────────────────────
# STATIC = compiled once into a .a archive, then linked into agent and server.
# No Dockerfile, no executable — this is purely a library.
add_library(common_lib STATIC
    src/convert_endian.cpp
    src/frame_transport.cpp
    src/env_helper.cpp
    src/logger.cpp
    src/dispatcher/dispatcher.cpp
    src/protocol/protocol_parser.cpp
    src/protocol/protocol_serializer.cpp
    src/protocol/metrics_serializer.cpp
    src/protocol/metrics_parser.cpp
    src/socket/linux_socket.cpp
    src/socket/socket_factory.cpp
    src/socket/tls_socket.cpp
    src/socket/tls_socket_factory.cpp
    src/protocol/protocol_helper.cpp
    src/poller/poller.cpp
)

target_link_libraries(common_lib PUBLIC OpenSSL::SSL OpenSSL::Crypto)

# PUBLIC = these include dirs are inherited by any target that links common_lib.
# This is what lets agent and server write #include "ISocket.hpp" without paths.
target_include_directories(common_lib PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_compile_options(common_lib PRIVATE
    -Wall -Wextra -Wpedantic
)

# ── Tests ─────────────────────────────────────────────────────────────────────
if(CMAKE_PROJECT_NAME STREQUAL "InfernoCommon")
    enable_testing()
    find_package(GTest REQUIRED)

    add_subdirectory(
        ${CMAKE_CURRENT_SOURCE_DIR}/../test_support
        ${CMAKE_CURRENT_BINARY_DIR}/test_support
    )

    set(PARSER_TESTS
        tests/protocol/parser/protocol_parser_test.cpp
    )

    set(SERIALIZER_TESTS
        tests/protocol/serializer/protocol_serializer_test.cpp
    )

    set(PARSER_SERIALIZER_TESTS
        tests/protocol/protocol_roundtrip_test.cpp
        tests/protocol/round_trip_metrics_test.cpp
        tests/protocol/round_trip_serialization_test.cpp
    )

    set(SOCKET_TESTS
        tests/socket/socket_integration_test.cpp
        tests/socket/tls_socket_integration_test.cpp
        )
        
        add_executable(common_tests
        tests/convert_endian_test.cpp
        tests/frame_transport_test.cpp
        ${PARSER_SERIALIZER_TESTS}
        ${PARSER_TESTS}
        ${SERIALIZER_TESTS}
        ${SOCKET_TESTS}
    )

    target_include_directories(common_tests PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/tests
    )

    target_link_libraries(common_tests PRIVATE
        common_lib
        test_support
        GTest::gtest
        GTest::gmock
        GTest::gtest_main
    )

    target_compile_options(common_tests PRIVATE
        -Wall -Wextra -Wpedantic
    )
    
    add_test(NAME common_tests COMMAND common_tests)
    set_tests_properties(common_tests PROPERTIES
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}/..
    )
    # include(GoogleTest)
    # gtest_discover_tests(common_tests DISCOVERY_MODE PRE_TEST)

endif()