#ifndef PROTOCOL_HEADER_HPP
#define PROTOCOL_HEADER_HPP
#include <array>
#include <vector>
#include <cstdint>
#include <string_view>

constexpr std::uint8_t LPTF_VERSION = 1;

constexpr std::uint8_t LPTF_HEADER_SIZE =
    sizeof(std::uint8_t) * 6 +
    sizeof(std::uint16_t);  // identifier + version + type + size

constexpr std::array<char, 4> LPTF_IDENTIFIER = {'L', 'P', 'T', 'F'};
constexpr std::string_view LPTF_IDENTIFIER_STR(LPTF_IDENTIFIER.data(), 4);

enum class MessageType : std::uint8_t {
  REGISTER = 0,
  DATA = 1,
  COMMAND = 2,
  RESPONSE = 3,
  DISCONNECT = 4,
  ERROR = 5,
  END,  // must always be the last !!
};

struct LptfHeader {
  // char identifier[4];
  std::array<char, 4> identifier = LPTF_IDENTIFIER;
  std::uint8_t version = LPTF_VERSION;
  MessageType type = MessageType::END;
  std::uint16_t size = LPTF_HEADER_SIZE;

  bool operator==(const LptfHeader& other) const {
    return std::equal(std::begin(identifier), std::end(identifier),
                      std::begin(other.identifier)) &&
           version == other.version && type == other.type && size == other.size;
    // return identifier == other.identifier && version == other.version &&
    //        type == other.type && size == other.size;
  }
};

// Generic struct for recv()
struct Frame {
  LptfHeader header;
  std::vector<std::uint8_t> payload = {};

  bool operator==(const Frame& other) const {
    return header == other.header && payload == other.payload;
  }
};

#endif