#ifndef C1_TEST_HELPERS_HPP
#define C1_TEST_HELPERS_HPP

#include <cstdint>
#include <string>
#include <vector>

#include "protocol/lptf_protocol.hpp"
#include "protocol/protocol_helper.hpp"
#include "protocol/protocol_serializer.hpp"
#include "test_constants.hpp"
#include "convert_endian.hpp"


inline std::vector<std::uint8_t> bytesFromString(const std::string& s) {
  return std::vector<std::uint8_t>(s.begin(), s.end());
}

inline Frame makeFrame(MessageType type,
                       const std::vector<std::uint8_t>& payload = {}) {
  return {ProtocolHelper::createHeader(type, payload), payload};
}

// ── Raw frame builder ─────────────────────────────────────────
// Builds a complete wire-format frame (header + payload).
// Intentionally does NOT use ProtocolParser so that
// AgentSession tests are independent of the parser.
inline std::vector<std::uint8_t> makeRawFrame(
    MessageType type, const std::vector<std::uint8_t>& payload = {}) {
  const std::uint16_t size = static_cast<std::uint16_t>(payload.size());
  std::vector<std::uint8_t> frame = {
      'L',
      'P',
      'T',
      'F',
      LPTF_VERSION,
      static_cast<std::uint8_t>(type),
      static_cast<std::uint8_t>(size >> 8),
      static_cast<std::uint8_t>(size & 0xFF),
  };
  frame.insert(frame.end(), payload.begin(), payload.end());
  return frame;
}

// ── Typed payload builders (use serializer) ───────────────────

// inline std::vector<std::uint8_t> makeRegisterPayload(
//     const std::string& hostname, OSType os = OSType::LINUX,
//     ArchType arch = ArchType::X64,
//     const std::string& osVersion = "Test OS 1.0",
//     const std::string& currentUser = "testuser") {
//   RegisterPayload p;
//   p.os_type = os;
//   p.arch = arch;
//   p.hostname = hostname;
//   p.os_version = osVersion;
//   p.current_user = currentUser;
//   return ProtocolSerializer::serializeRegisterPayload(p);
// }

inline std::vector<std::uint8_t> makeRegisterPayload(
    const std::uint8_t rawOs = static_cast<uint8_t>(OSType::LINUX),
    const std::uint8_t rawArch = static_cast<uint8_t>(ArchType::X64),
    const std::uint16_t declaredHostnameLen = TestConstants::TEST_HOSTNAME_LEN,
    const std::uint16_t declaredOsVersionLen =
        TestConstants::TEST_OS_VERSION_LEN,
    const std::uint16_t declaredCurrentUserLen =
        TestConstants::TEST_CURRENT_USER_LEN,
    const std::vector<std::uint8_t>& hostnameBytes =
        TestConstants::TEST_HOSTNAME,
    const std::vector<std::uint8_t>& osVersionBytes =
        TestConstants::TEST_OS_VERSION,
    const std::vector<std::uint8_t>& currentUserBytes =
        TestConstants::TEST_CURRENT_USER) {
  std::vector<std::uint8_t> out(REGISTER_FIXED_BYTES);

  out[0] = rawOs;
  out[1] = rawArch;
  ConvertEndian::writeU16BE(out, 2, declaredHostnameLen);
  ConvertEndian::writeU16BE(out, 4, declaredOsVersionLen);
  ConvertEndian::writeU16BE(out, 6, declaredCurrentUserLen);

  out.insert(out.end(), hostnameBytes.begin(), hostnameBytes.end());
  out.insert(out.end(), osVersionBytes.begin(), osVersionBytes.end());
  out.insert(out.end(), currentUserBytes.begin(), currentUserBytes.end());
  return out;
}

inline std::vector<std::uint8_t> makeResponsePayload(
    std::uint16_t id, const std::string& data, std::uint8_t totalChunks = 1,
    std::uint8_t chunkIndex = 0) {
  ResponsePayload p;
  p.id = id;
  p.status = ResponseStatus::OK;
  p.total_chunks = totalChunks;
  p.chunk_index = chunkIndex;
  p.data = data;
  return ProtocolSerializer::serializeResponsePayload(p);
}

#endif
