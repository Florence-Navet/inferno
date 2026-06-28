#include <gtest/gtest.h>

#include "builders/frame_builder.hpp"
#include "convert_endian.hpp"
#include "exception/lptf_exception.hpp"
#include "fixtures/common.hpp"
#include "fixtures/protocol.hpp"
#include "protocol/lptf_protocol.hpp"
#include "protocol/protocol_parser.hpp"

// ── Happy path ────────────────────────────────────────────────────────────────

TEST(ProtocolParserRegister,
     should_parse_register_payload_when_input_is_valid) {
  const std::vector<std::uint8_t> input = FrameBuilder::makeRegisterPayload();

  const RegisterPayload result = ProtocolParser::parseRegisterPayload(input);

  EXPECT_EQ(result.os_type, OSType::LINUX);
  EXPECT_EQ(result.arch, ArchType::X64);
  EXPECT_EQ(result.hostname, Protocol::TEST_HOSTNAME_STR);
  EXPECT_EQ(result.os_version, Protocol::TEST_OS_VERSION_STR);
  EXPECT_EQ(result.current_user, Protocol::TEST_CURRENT_USER_STR);
}

TEST(ProtocolParserRegister, should_accept_max_hostname_length) {
  const size_t maxHostnameLen = REGISTER_MAX_HOSTNAME_LEN -
                                Protocol::TEST_OS_VERSION_LEN -
                                Protocol::TEST_CURRENT_USER_LEN;
  const std::string longHostname(maxHostnameLen, 'a');

  const std::vector<std::uint8_t> input = FrameBuilder::makeRegisterPayload(
      static_cast<std::uint8_t>(OSType::MAC),
      static_cast<std::uint8_t>(ArchType::ARM),
      static_cast<std::uint16_t>(longHostname.size()),
      Protocol::TEST_OS_VERSION_LEN,
      Protocol::TEST_CURRENT_USER_LEN,
      Common::bytesFromString(longHostname));

  const RegisterPayload result = ProtocolParser::parseRegisterPayload(input);

  EXPECT_EQ(result.os_type, OSType::MAC);
  EXPECT_EQ(result.arch, ArchType::ARM);
  EXPECT_EQ(result.hostname.size(), maxHostnameLen);
  EXPECT_EQ(result.hostname.front(), 'a');
  EXPECT_EQ(result.hostname.back(), 'a');
}

TEST(ProtocolParserRegister, should_decode_hostname_length_as_big_endian) {
  const std::string longHostname(300, 'z');

  const std::vector<std::uint8_t> input = FrameBuilder::makeRegisterPayload(
      static_cast<std::uint8_t>(OSType::WINDOWS),
      static_cast<std::uint8_t>(ArchType::X86),
      300,
      Protocol::TEST_OS_VERSION_LEN,
      Protocol::TEST_CURRENT_USER_LEN,
      Common::bytesFromString(longHostname));

  const RegisterPayload result = ProtocolParser::parseRegisterPayload(input);

  EXPECT_EQ(result.hostname.size(), 300u);
  EXPECT_EQ(result.hostname[0], 'z');
  EXPECT_EQ(result.hostname[299], 'z');
}

// ── Structural / size errors ──────────────────────────────────────────────────

TEST(ProtocolParserRegister,
     should_reject_payload_shorter_than_fixed_register_fields) {
  const std::vector<std::uint8_t> input = {
      static_cast<std::uint8_t>(OSType::LINUX),
      static_cast<std::uint8_t>(ArchType::X64),
      0x00};

  EXPECT_THROW(ProtocolParser::parseRegisterPayload(input), InvalidSize);
}

TEST(ProtocolParserRegister, should_reject_empty_hostname) {
  const std::vector<std::uint8_t> input = FrameBuilder::makeRegisterPayload(
      static_cast<std::uint8_t>(OSType::WINDOWS),
      static_cast<std::uint8_t>(ArchType::X86),
      0x00,
      Protocol::TEST_OS_VERSION_LEN,
      Protocol::TEST_CURRENT_USER_LEN,
      {});

  EXPECT_THROW(ProtocolParser::parseRegisterPayload(input), InvalidSize);
}

TEST(ProtocolParserRegister, should_reject_empty_os_version) {
  const std::vector<std::uint8_t> input = FrameBuilder::makeRegisterPayload(
      static_cast<std::uint8_t>(OSType::LINUX),
      static_cast<std::uint8_t>(ArchType::X64),
      Protocol::TEST_HOSTNAME_LEN,
      0,
      Protocol::TEST_CURRENT_USER_LEN,
      Protocol::TEST_HOSTNAME,
      {},
      Protocol::TEST_CURRENT_USER);

  EXPECT_THROW(ProtocolParser::parseRegisterPayload(input), InvalidSize);
}

TEST(ProtocolParserRegister, should_reject_empty_current_user) {
  const std::vector<std::uint8_t> input = FrameBuilder::makeRegisterPayload(
      static_cast<std::uint8_t>(OSType::LINUX),
      static_cast<std::uint8_t>(ArchType::X64),
      Protocol::TEST_HOSTNAME_LEN,
      Protocol::TEST_OS_VERSION_LEN,
      0,
      Protocol::TEST_HOSTNAME,
      Protocol::TEST_OS_VERSION,
      {});

  EXPECT_THROW(ProtocolParser::parseRegisterPayload(input), InvalidSize);
}

TEST(ProtocolParserRegister,
     should_reject_when_declared_hostname_length_exceeds_available_bytes) {
  const std::vector<std::uint8_t> input = FrameBuilder::makeRegisterPayload(
      static_cast<std::uint8_t>(OSType::LINUX),
      static_cast<std::uint8_t>(ArchType::X64),
      10,
      Protocol::TEST_OS_VERSION_LEN,
      Protocol::TEST_CURRENT_USER_LEN,
      Common::bytesFromString("abc"),  // only 3 bytes
      Protocol::TEST_OS_VERSION,
      Protocol::TEST_CURRENT_USER);

  EXPECT_THROW(ProtocolParser::parseRegisterPayload(input), InvalidSize);
}

// ── Invalid enum values ───────────────────────────────────────────────────────

TEST(ProtocolParserRegister, should_reject_unknown_os_type) {
  const std::vector<std::uint8_t> input = FrameBuilder::makeRegisterPayload(
      Common::INVALID_ENUM_VALUE,
      static_cast<std::uint8_t>(ArchType::X64),
      Protocol::TEST_HOSTNAME_LEN,
      Protocol::TEST_OS_VERSION_LEN,
      Protocol::TEST_CURRENT_USER_LEN,
      Protocol::TEST_HOSTNAME,
      Protocol::TEST_OS_VERSION,
      Protocol::TEST_CURRENT_USER);

  EXPECT_THROW(ProtocolParser::parseRegisterPayload(input), InvalidFieldValue);
}

TEST(ProtocolParserRegister, should_reject_unknown_arch) {
  const std::vector<std::uint8_t> input = FrameBuilder::makeRegisterPayload(
      static_cast<std::uint8_t>(OSType::LINUX),
      Common::INVALID_ENUM_VALUE,
      Protocol::TEST_HOSTNAME_LEN,
      Protocol::TEST_OS_VERSION_LEN,
      Protocol::TEST_CURRENT_USER_LEN,
      Protocol::TEST_HOSTNAME,
      Protocol::TEST_OS_VERSION,
      Protocol::TEST_CURRENT_USER);

  EXPECT_THROW(ProtocolParser::parseRegisterPayload(input), InvalidFieldValue);
}