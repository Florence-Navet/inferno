#ifndef CONVERT_ENDIAN_HPP
#define CONVERT_ENDIAN_HPP

#include <cstddef>
#include <cstdint>
#include <vector>
#include <string>

// INFO what's the difference between std::uint8_t and uint8_t OR
// std::size_t and std::size_t...etc
// std::uint8_t means “take uint8_t from namespace std”. Better.
// uint8_t without std:: relies on it being available in global namespace on
// your compiler/environment.
class ConvertEndian {
 public:
  ConvertEndian() = delete;
  ConvertEndian(const ConvertEndian&) = delete;
  ConvertEndian& operator=(const ConvertEndian&) = delete;

  // static std::uint16_t readU16BE(const std::vector<std::uint8_t>& buffer,
  //                                std::size_t offset) {
  //   // high byte = offset, low byte = offset + 1
  //   return static_cast<std::uint16_t>(buffer[offset] << 8 | buffer[offset +
  //   1]);
  // };

  static std::uint16_t readU16BE(const std::vector<std::uint8_t>& buffer,
                                 std::size_t& offset);

  static void writeU16BE(std::vector<std::uint8_t>& buffer, std::size_t offset,
                         std::uint16_t value);

  static std::uint32_t readU32BE(const std::vector<std::uint8_t>& buffer,
                                 std::size_t& offset);

  static std::uint64_t readU64BE(const std::vector<std::uint8_t>& buffer,
                                 std::size_t& offset);

  static float readFloat(const std::vector<std::uint8_t>& buffer,
                         std::size_t& offset);

  static std::string getString(const std::vector<std::uint8_t>& buffer,
                               std::size_t& offset, std::uint16_t length);
};

#endif