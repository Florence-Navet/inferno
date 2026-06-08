#include <gtest/gtest.h>

#include "protocol/lptf_protocol.hpp"
#include "protocol/protocol_parser.hpp"
#include "protocol/protocol_test_helpers.hpp"
#include "convert_endian.hpp"
#include "exception/lptf_exception.hpp"

namespace {

// ---------------------------------------------------------------------------
// Helpers
// ---------------------------------------------------------------------------

std::vector<std::uint8_t> makeRegisterPayload(
    const std::uint8_t rawOs,
    const std::uint8_t rawArch,
    const std::uint16_t declaredHostnameLen,
    const std::uint16_t declaredOsVersionLen,
    const std::uint16_t declaredCurrentUserLen,
    const std::vector<std::uint8_t>& hostnameBytes,
    const std::vector<std::uint8_t>& osVersionBytes,
    const std::vector<std::uint8_t>& currentUserBytes) {

  std::vector<std::uint8_t> out;
  out.push_back(rawOs);
  out.push_back(rawArch);

  out.push_back(static_cast<std::uint8_t>((declaredHostnameLen >> 8) & 0xFF));
  out.push_back(static_cast<std::uint8_t>(declaredHostnameLen & 0xFF));

  out.push_back(static_cast<std::uint8_t>((declaredOsVersionLen >> 8) & 0xFF));
  out.push_back(static_cast<std::uint8_t>(declaredOsVersionLen & 0xFF));

  out.push_back(static_cast<std::uint8_t>((declaredCurrentUserLen >> 8) & 0xFF));
  out.push_back(static_cast<std::uint8_t>(declaredCurrentUserLen & 0xFF));

  out.insert(out.end(), hostnameBytes.begin(), hostnameBytes.end());
  out.insert(out.end(), osVersionBytes.begin(), osVersionBytes.end());
  out.insert(out.end(), currentUserBytes.begin(), currentUserBytes.end());
  return out;
}

std::vector<std::uint8_t> bytesFromString(const std::string& s) {
  return std::vector<std::uint8_t>(s.begin(), s.end());
}

// Reused across tests so every field is always non-zero unless we are
// specifically testing that field being invalid.
const std::string kHostname    = "agent-01";
const std::string kOsVersion   = "Ubuntu 22.04";
const std::string kCurrentUser = "root";

}  // namespace

// ---------------------------------------------------------------------------
// Happy path
// ---------------------------------------------------------------------------

TEST(ProtocolParserRegister, should_parse_register_payload_when_input_is_valid) {
  const std::vector<std::uint8_t> input = makeRegisterPayload(
      static_cast<std::uint8_t>(OSType::LINUX),
      static_cast<std::uint8_t>(ArchType::X64),
      static_cast<std::uint16_t>(kHostname.size()),
      static_cast<std::uint16_t>(kOsVersion.size()),
      static_cast<std::uint16_t>(kCurrentUser.size()),
      bytesFromString(kHostname),
      bytesFromString(kOsVersion),
      bytesFromString(kCurrentUser));

  const RegisterPayload result = ProtocolParser::parseRegisterPayload(input);

  EXPECT_EQ(result.os_type,      OSType::LINUX);
  EXPECT_EQ(result.arch,         ArchType::X64);
  EXPECT_EQ(result.hostname,     kHostname);
  EXPECT_EQ(result.os_version,   kOsVersion);
  EXPECT_EQ(result.current_user, kCurrentUser);
}

TEST(ProtocolParserRegister, should_accept_max_hostname_length) {
  const std::string longHostname(REGISTER_MAX_HOSTNAME_LEN, 'a');

  const std::vector<std::uint8_t> input = makeRegisterPayload(
      static_cast<std::uint8_t>(OSType::MAC),
      static_cast<std::uint8_t>(ArchType::ARM),
      static_cast<std::uint16_t>(longHostname.size()),
      static_cast<std::uint16_t>(kOsVersion.size()),
      static_cast<std::uint16_t>(kCurrentUser.size()),
      bytesFromString(longHostname),
      bytesFromString(kOsVersion),
      bytesFromString(kCurrentUser));

  const RegisterPayload result = ProtocolParser::parseRegisterPayload(input);

  EXPECT_EQ(result.os_type,          OSType::MAC);
  EXPECT_EQ(result.arch,             ArchType::ARM);
  EXPECT_EQ(result.hostname.size(),  REGISTER_MAX_HOSTNAME_LEN);
  EXPECT_EQ(result.hostname.front(), 'a');
  EXPECT_EQ(result.hostname.back(),  'a');
}

// hostname_len is 300 — value > 255 must be decoded as big-endian uint16
TEST(ProtocolParserRegister, should_decode_hostname_length_as_big_endian) {
  const std::string longHostname(300, 'z');

  const std::vector<std::uint8_t> input = makeRegisterPayload(
      static_cast<std::uint8_t>(OSType::WINDOWS),
      static_cast<std::uint8_t>(ArchType::X86),
      300,
      static_cast<std::uint16_t>(kOsVersion.size()),
      static_cast<std::uint16_t>(kCurrentUser.size()),
      bytesFromString(longHostname),
      bytesFromString(kOsVersion),
      bytesFromString(kCurrentUser));

  const RegisterPayload result = ProtocolParser::parseRegisterPayload(input);

  EXPECT_EQ(result.hostname.size(), 300u);
  EXPECT_EQ(result.hostname[0],     'z');
  EXPECT_EQ(result.hostname[299],   'z');
}

// ---------------------------------------------------------------------------
// Structural / size errors
// ---------------------------------------------------------------------------

TEST(ProtocolParserRegister, should_reject_payload_shorter_than_fixed_register_fields) {
  // 3 bytes — cannot even hold the 8-byte fixed header
  const std::vector<std::uint8_t> input = {
      static_cast<std::uint8_t>(OSType::LINUX),
      static_cast<std::uint8_t>(ArchType::X64),
      0x00};

  EXPECT_THROW(ProtocolParser::parseRegisterPayload(input), InvalidSize);
}

TEST(ProtocolParserRegister, should_reject_empty_hostname) {
  // hostname declared as 0 — must be rejected by validateNotNullLength
  const std::vector<std::uint8_t> input = makeRegisterPayload(
      static_cast<std::uint8_t>(OSType::WINDOWS),
      static_cast<std::uint8_t>(ArchType::X86),
      0,                                                    // hostname_len = 0
      static_cast<std::uint16_t>(kOsVersion.size()),
      static_cast<std::uint16_t>(kCurrentUser.size()),
      {},
      bytesFromString(kOsVersion),
      bytesFromString(kCurrentUser));

  EXPECT_THROW(ProtocolParser::parseRegisterPayload(input), InvalidSize);
}

TEST(ProtocolParserRegister, should_reject_empty_os_version) {
  const std::vector<std::uint8_t> input = makeRegisterPayload(
      static_cast<std::uint8_t>(OSType::LINUX),
      static_cast<std::uint8_t>(ArchType::X64),
      static_cast<std::uint16_t>(kHostname.size()),
      0,                                                    // os_version_len = 0
      static_cast<std::uint16_t>(kCurrentUser.size()),
      bytesFromString(kHostname),
      {},
      bytesFromString(kCurrentUser));

  EXPECT_THROW(ProtocolParser::parseRegisterPayload(input), InvalidSize);
}

TEST(ProtocolParserRegister, should_reject_empty_current_user) {
  const std::vector<std::uint8_t> input = makeRegisterPayload(
      static_cast<std::uint8_t>(OSType::LINUX),
      static_cast<std::uint8_t>(ArchType::X64),
      static_cast<std::uint16_t>(kHostname.size()),
      static_cast<std::uint16_t>(kOsVersion.size()),
      0,                                                    // current_user_len = 0
      bytesFromString(kHostname),
      bytesFromString(kOsVersion),
      {});

  EXPECT_THROW(ProtocolParser::parseRegisterPayload(input), InvalidSize);
}

// Declared hostname length is larger than the bytes actually present
TEST(ProtocolParserRegister, should_reject_when_declared_hostname_length_exceeds_available_bytes) {
  const std::vector<std::uint8_t> input = makeRegisterPayload(
      static_cast<std::uint8_t>(OSType::LINUX),
      static_cast<std::uint8_t>(ArchType::X64),
      10,                                                   // claims 10 bytes
      static_cast<std::uint16_t>(kOsVersion.size()),
      static_cast<std::uint16_t>(kCurrentUser.size()),
      bytesFromString("abc"),                               // only 3 bytes
      bytesFromString(kOsVersion),
      bytesFromString(kCurrentUser));

  EXPECT_THROW(ProtocolParser::parseRegisterPayload(input), InvalidSize);
}

// ---------------------------------------------------------------------------
// Invalid enum values
// ---------------------------------------------------------------------------

TEST(ProtocolParserRegister, should_reject_unknown_os_type) {
  const std::vector<std::uint8_t> input = makeRegisterPayload(
      TestHelpers::INVALID_ENUM_VALUE,
      static_cast<std::uint8_t>(ArchType::X64),
      static_cast<std::uint16_t>(kHostname.size()),
      static_cast<std::uint16_t>(kOsVersion.size()),
      static_cast<std::uint16_t>(kCurrentUser.size()),
      bytesFromString(kHostname),
      bytesFromString(kOsVersion),
      bytesFromString(kCurrentUser));

  EXPECT_THROW(ProtocolParser::parseRegisterPayload(input), InvalidFieldValue);
}

TEST(ProtocolParserRegister, should_reject_unknown_arch) {
  const std::vector<std::uint8_t> input = makeRegisterPayload(
      static_cast<std::uint8_t>(OSType::LINUX),
      TestHelpers::INVALID_ENUM_VALUE,
      static_cast<std::uint16_t>(kHostname.size()),
      static_cast<std::uint16_t>(kOsVersion.size()),
      static_cast<std::uint16_t>(kCurrentUser.size()),
      bytesFromString(kHostname),
      bytesFromString(kOsVersion),
      bytesFromString(kCurrentUser));

  EXPECT_THROW(ProtocolParser::parseRegisterPayload(input), InvalidFieldValue);
}