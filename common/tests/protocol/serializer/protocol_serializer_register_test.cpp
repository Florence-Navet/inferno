#include <gtest/gtest.h>

#include <string>
#include <vector>

#include "builders/frame_builder.hpp"
#include "exception/lptf_exception.hpp"
#include "fixtures/common.hpp"
#include "fixtures/protocol.hpp"
#include "protocol/lptf_protocol.hpp"
#include "protocol/protocol_serializer.hpp"

TEST(ProtocolSerializerRegister,
     should_produce_corresponding_byteArray_when_OsInfoPayload_is_valid) {
  // Arrange
  //   const OsInfoPayload input{
  //       OSType::LINUX, ArchType::X64, Protocol::TEST_HOSTNAME_STR,
  //       Protocol::TEST_OS_VERSION_STR, Protocol::TEST_CURRENT_USER_STR};
  const OsInfoPayload input = FrameBuilder::makeOsInfoPayload();

  const std::vector<std::uint8_t> expected =
      FrameBuilder::makeRawOsInfoPayload();

  // Act
  const std::vector<std::uint8_t> result =
      ProtocolSerializer::serializeOsInfoPayload(input);

  // Assert
  EXPECT_EQ(expected, result);
}

TEST(ProtocolSerializerRegister,
     should_throw_InvalidSize_when_hostname_is_empty) {
  const OsInfoPayload input{OSType::WINDOWS, ArchType::X86, "",
                            Protocol::TEST_OS_VERSION_STR,
                            Protocol::TEST_CURRENT_USER_STR};

  EXPECT_THROW(ProtocolSerializer::serializeOsInfoPayload(input), InvalidSize);
}

TEST(ProtocolSerializerRegister,
     should_throw_InvalidFieldValue_when_os_type_is_unknown) {
  // Arrange
  const OsInfoPayload input{
      static_cast<OSType>(Common::INVALID_ENUM_VALUE), ArchType::X64, "host",
      Protocol::TEST_OS_VERSION_STR, Protocol::TEST_CURRENT_USER_STR};

  // Act & Assert
  EXPECT_THROW(ProtocolSerializer::serializeOsInfoPayload(input),
               InvalidFieldValue);
}

TEST(ProtocolSerializerRegister,
     should_throw_InvalidFieldValue_when_arch_is_unknown) {
  // Arrange
  const OsInfoPayload input{
      OSType::LINUX, static_cast<ArchType>(Common::INVALID_ENUM_VALUE), "host",
      Protocol::TEST_OS_VERSION_STR, Protocol::TEST_CURRENT_USER_STR};

  // Act & Assert
  EXPECT_THROW(ProtocolSerializer::serializeOsInfoPayload(input),
               InvalidFieldValue);
}

TEST(ProtocolSerializerRegister,
     should_throw_InvalidSize_when_hostname_size_exceeds_max) {
  // Arrange
  //   const std::uint16_t maxLen = REGISTER_MAX_HOSTNAME_LEN -
  //                                Protocol::TEST_OS_VERSION_LEN -
  //                                Protocol::TEST_CURRENT_USER_LEN;
  const OsInfoPayload input{OSType::LINUX, ArchType::X64,
                            std::string(REGISTER_MAX_HOSTNAME_LEN + 1, 'a'),
                            Protocol::TEST_OS_VERSION_STR,
                            Protocol::TEST_CURRENT_USER_STR};

  // Act & Assert
  EXPECT_THROW(ProtocolSerializer::serializeOsInfoPayload(input), InvalidSize);
}
