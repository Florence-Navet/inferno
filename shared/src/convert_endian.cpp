#include "convert_endian.hpp"

#include <cstring>

#ifdef _WIN32
#include <winsock2.h>
#define be64toh(x) _byteswap_uint64(x)
#else
#include <arpa/inet.h>
#include <endian.h>
#endif

void ConvertEndian::writeU16BE(std::vector<std::uint8_t>& buffer,
                               std::size_t offset, std::uint16_t value) {
  //  00000001 00101100 = 300 in big endian

  const std::uint8_t low = static_cast<std::uint8_t>(value & 0xFF);
  //   00000001 00101100
  // & 00000000 11111111
  //   00000000 00101100 // here is the least significant

  const std::uint8_t high = static_cast<std::uint8_t>((value >> 8) & 0xFF);
  //   00000001 00101100 >> 8 => 00000000 00000001
  //   00000000 00000001
  // & 00000000 11111111
  //   00000000 00000001

  buffer[offset] = high;
  buffer[offset + 1] = low;
}

// std::uint16_t ConvertEndian::readU16BE(const std::vector<std::uint8_t>&
// buffer,
//                                        std::size_t offset) {

//   return static_cast<std::uint16_t>(buffer[offset] << 8 | buffer[offset +
//   1]);
// };

std::uint16_t ConvertEndian::readU16BE(const std::vector<std::uint8_t>& buffer,
                                       std::size_t& offset) {  // offset by ref
  // high byte = offset, low byte = offset + 1
  std::uint16_t value =
      static_cast<std::uint16_t>(buffer[offset] << 8 | buffer[offset + 1]);
  offset += sizeof(std::uint16_t);
  return value;
}

std::uint32_t ConvertEndian::readU32BE(const std::vector<std::uint8_t>& buffer,
                                       std::size_t& offset) {
  uint32_t raw_value;
  std::memcpy(&raw_value, buffer.data() + offset, sizeof(raw_value));
  offset += sizeof(uint32_t);
  return ntohl(raw_value);
}

std::uint64_t ConvertEndian::readU64BE(const std::vector<std::uint8_t>& buffer,
                                       std::size_t& offset) {
  uint64_t raw_mem;
  std::memcpy(&raw_mem, buffer.data() + offset, sizeof(raw_mem));
  offset += sizeof(uint64_t);
  return be64toh(raw_mem);
}

float ConvertEndian::readFloat(const std::vector<std::uint8_t>& buffer,
                               std::size_t& offset) {
  float value;
  std::uint32_t raw_float = ConvertEndian::readU32BE(buffer, offset);
  std::memcpy(&value, &raw_float, sizeof(value));
  return value;
}

std::string ConvertEndian::getString(const std::vector<std::uint8_t>& buffer,
                                     std::size_t& offset,
                                     std::uint16_t length) {
  std::string finalString = std::string(
      reinterpret_cast<const char*>(buffer.data() + offset), length);
  offset += length;
  return finalString;
}