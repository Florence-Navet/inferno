// #include <gtest/gtest.h>

// #include <string>
// #include <vector>

// #include "exception/lptf_exception.hpp"
// #include "helpers_test.hpp"
// #include "protocol/lptf_protocol.hpp"
// #include "protocol/protocol_serializer.hpp"
// #include "protocol/protocol_test_helpers.hpp"

// ResponsePayload input FrameBuilder::makeResponsePayload(9,
// ResponseStatus::OK, 2, 1,
//                                           "chunk");

// TEST(ProtocolSerializerResponse,
//      should_produce_corresponding_byteArray_when_response_is_valid) {
//   // Arrange
//   //   const ResponsePayload input{9, ResponseStatus::OK, 2, 1,
//   //   {"c","h","u","n","k"}};
//   //   const ResponsePayload input =
//   //       FrameBuilder::makeResponsePayload(9, ResponseStatus::OK, 2, 1,
//   "chunk")
//   //   std::uint16_t id, const std::string& data, std::uint8_t totalChunks =
//   //   1,
//   //     std::uint8_t chunkIndex =
//   const std::vector<std::uint8_t> expected = {
//       0x00, 0x09, static_cast<std::uint8_t>(ResponseStatus::OK),
//       0x02, 0x01, 0x00,
//       0x05, 'c',  'h',
//       'u',  'n',  'k'};

//   // Act
//   const std::vector<std::uint8_t> result =
//       ProtocolSerializer::serializeResponsePayload(input);

//   // Assert
//   EXPECT_EQ(expected, result);
// }

// TEST(ProtocolSerializerResponse,
//      should_throw_InvalidFieldValue_when_status_is_unknown) {
//   // Arrange
//   //   const ResponsePayload input{
//   //       9, static_cast<ResponseStatus>(TestHelpers::INVALID_ENUM_VALUE),
//   1,
//   //       0,
//   //       {""}};

//   input.status =
//   static_cast<ResponseStatus>(TestHelpers::INVALID_ENUM_VALUE);

//   // Act & Assert
//   EXPECT_THROW(ProtocolSerializer::serializeResponsePayload(input),
//                InvalidFieldValue);
// }

// TEST(ProtocolSerializerResponse,
//      should_throw_InvalidFieldValue_when_total_chunks_is_zero) {
//   // Arrange
//   //   const ResponsePayload input{9, ResponseStatus::OK, 0, 0, {""}};
//   input.total_chunk = 0;
//   input.chunk_index = 0;
//   input.data = {};

//   // Act & Assert
//   EXPECT_THROW(ProtocolSerializer::serializeResponsePayload(input),
//                InvalidFieldValue);
// }

// TEST(ProtocolSerializerResponse,
//      should_throw_InvalidFieldValue_when_chunk_index_is_out_of_range) {
//   // Arrange
//   //   const ResponsePayload input{9, ResponseStatus::OK, 2, 2, {""}};
//   input.total_chunk = 2;
//   input.index = 2;
//   // Act & Assert
//   EXPECT_THROW(ProtocolSerializer::serializeResponsePayload(input),
//                InvalidFieldValue);
// }

// TEST(ProtocolSerializerResponse,
//      should_throw_InvalidSize_when_response_data_is_too_large) {
//   // Arrange
//   //   const ResponsePayload input{
//   //       9, ResponseStatus::OK, 1, 0,
//   //       std::string(static_cast<std::size_t>(65535) + 1, 'a')};
//   std::string testString(static_cast<std::size_t>(65535) + 1, 'a');
//   input.data = testString;
//   // Act & Assert
//   EXPECT_THROW(ProtocolSerializer::serializeResponsePayload(input),
//                InvalidSize);
// }
