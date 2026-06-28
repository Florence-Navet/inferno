#include <gtest/gtest.h>

#include <string>
#include <vector>

#include "exception/lptf_exception.hpp"
#include "fixtures/common.hpp"
#include "fixtures/protocol.hpp"
#include "builders/frame_builder.hpp"
#include "protocol/lptf_protocol.hpp"
#include "protocol/protocol_serializer.hpp"

TEST(ProtocolSerializerRegister,
     should_produce_corresponding_byteArray_when_registerPayload_is_valid) {
  // Arrange
  const RegisterPayload input{
      OSType::LINUX, ArchType::X64, Protocol::TEST_HOSTNAME_STR,
      Protocol::TEST_OS_VERSION_STR, Protocol::TEST_CURRENT_USER_STR};

  const std::vector<std::uint8_t> expected = FrameBuilder::makeRegisterPayload(
      static_cast<std::uint8_t>(OSType::LINUX),
      static_cast<std::uint8_t>(ArchType::X64));

  //   const std::vector<std::uint8_t> expected = {
  //       static_cast<std::uint8_t>(OSType::LINUX),
  //       static_cast<std::uint8_t>(ArchType::X64),

  //       0x00, 0x08,  // hostname len
  //       0x00, 0x0C,  // os_version len
  //       0x00, 0x04,  // current_user len

  //       'a', 'g', 'e', 'n', 't', '-', '0', '1',

  //       'U', 'b', 'u', 'n', 't', 'u',
  //       ' ', '2', '2', '.', '0', '4',

  //       'r', 'o', 'o', 't'};

  // Act
  const std::vector<std::uint8_t> result =
      ProtocolSerializer::serializeRegisterPayload(input);

  // Assert
  EXPECT_EQ(expected, result);
}

TEST(ProtocolSerializerRegister,
     should_throw_InvalidSize_when_hostname_is_empty) {
  const RegisterPayload input{OSType::WINDOWS, ArchType::X86, "",
                              Protocol::TEST_OS_VERSION_STR,
                              Protocol::TEST_CURRENT_USER_STR};

  EXPECT_THROW(ProtocolSerializer::serializeRegisterPayload(input),
               InvalidSize);
}

TEST(ProtocolSerializerRegister,
     should_throw_InvalidFieldValue_when_os_type_is_unknown) {
  // Arrange
  const RegisterPayload input{
      static_cast<OSType>(Common::INVALID_ENUM_VALUE), ArchType::X64, "host",
      Protocol::TEST_OS_VERSION_STR, Protocol::TEST_CURRENT_USER_STR};

  // Act & Assert
  EXPECT_THROW(ProtocolSerializer::serializeRegisterPayload(input),
               InvalidFieldValue);
}

TEST(ProtocolSerializerRegister,
     should_throw_InvalidFieldValue_when_arch_is_unknown) {
  // Arrange
  const RegisterPayload input{
      OSType::LINUX, static_cast<ArchType>(Common::INVALID_ENUM_VALUE),
      "host", Protocol::TEST_OS_VERSION_STR, Protocol::TEST_CURRENT_USER_STR};

  // Act & Assert
  EXPECT_THROW(ProtocolSerializer::serializeRegisterPayload(input),
               InvalidFieldValue);
}

TEST(ProtocolSerializerRegister,
     should_throw_InvalidSize_when_hostname_size_exceeds_max) {
  // Arrange
//   const std::uint16_t maxLen = REGISTER_MAX_HOSTNAME_LEN -
//                                Protocol::TEST_OS_VERSION_LEN -
//                                Protocol::TEST_CURRENT_USER_LEN;
  const RegisterPayload input{
      OSType::LINUX, ArchType::X64, std::string(REGISTER_MAX_HOSTNAME_LEN + 1, 'a'),
      Protocol::TEST_OS_VERSION_STR, Protocol::TEST_CURRENT_USER_STR};

  // Act & Assert
  EXPECT_THROW(ProtocolSerializer::serializeRegisterPayload(input),
               InvalidSize);
}
