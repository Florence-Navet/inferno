#include <gtest/gtest.h>

#include <string>
#include <vector>

#include "exception/lptf_exception.hpp"
#include "fixtures/common.hpp"
#include "protocol/lptf_protocol.hpp"
#include "protocol/protocol_serializer.hpp"

TEST(ProtocolSerializerData,
     should_produce_corresponding_byteArray_when_data_payload_is_valid) {
  // Arrange
  //   const DataPayload input{DataType::METRICS_SAMPLE, "keys"};
  const DataPayload input{DataType::METRICS_SAMPLE, {'k', 'e', 'y', 's'}};
  const std::vector<std::uint8_t> expected = {
      static_cast<std::uint8_t>(DataType::METRICS_SAMPLE),
      0x00,
      0x04,
      'k',
      'e',
      'y',
      's'};

  // Act
  const std::vector<std::uint8_t> result =
      ProtocolSerializer::serializeDataPayload(input);

  // Assert
  EXPECT_EQ(expected, result);
}

TEST(ProtocolSerializerData,
     should_throw_InvalidFieldValue_when_data_subtype_is_unknown) {
  // Arrange
  const DataPayload input{static_cast<DataType>(Common::INVALID_ENUM_VALUE),
                          {}};

  // Act & Assert
  EXPECT_THROW(ProtocolSerializer::serializeDataPayload(input),
               InvalidFieldValue);
}

TEST(ProtocolSerializerData, should_throw_InvalidSize_when_data_is_too_large) {
  // Arrange
  const DataPayload input{DataType::METRICS_SAMPLE,
                          std::vector<uint8_t>(KMAX_U16_VALUE + 1, 'a')};

  // Act & Assert
  EXPECT_THROW(ProtocolSerializer::serializeDataPayload(input), InvalidSize);
}
