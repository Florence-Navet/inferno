#ifndef LPTF_SERIALIZER_HPP
#define LPTF_SERIALIZER_HPP

// #include <cstddef>
// #include <cstring>

#include <cstdint>
#include <vector>

#include "protocol/lptf_protocol.hpp"

class ProtocolSerializer {
 public:
  ProtocolSerializer() = delete;
  ProtocolSerializer(const ProtocolSerializer&) = delete;
  ProtocolSerializer& operator=(const ProtocolSerializer&) = delete;

  static std::vector<std::uint8_t> serializeHeader(const LptfHeader& header);
  static std::vector<std::uint8_t> serializeRegisterPayload(
      const RegisterPayload& payload);
  static std::vector<std::uint8_t> serializeCommandPayload(
      const CommandPayload& payload);
  static std::vector<std::uint8_t> serializeResponsePayload(
      const ResponsePayload& payload);
  static std::vector<std::uint8_t> serializeDataPayload(
      const DataPayload& payload);
  static std::vector<std::uint8_t> serializeErrorPayload(
      const ErrorPayload& payload);
  static inline std::vector<std::uint8_t> toBytes(const std::string& s) {
    return std::vector<std::uint8_t>(s.begin(), s.end());
  }
};

#endif