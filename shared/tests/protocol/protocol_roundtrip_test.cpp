// #include <gtest/gtest.h>

// #include <string>
// #include <vector>

// #include "protocol/lptf_protocol.hpp"
// #include "protocol/protocol_parser.hpp"
// #include "protocol/protocol_serializer.hpp"
// #include "test_constants.hpp"

// ResponsePayload responsePayloadInput makeResponsePayload(9, ResponseStatus::OK, 3, 1,
//                                           "chunk");

// TEST(ProtocolRoundTrip,
//      should_preserve_register_payload_through_serialize_then_parse) {
//   // Arrange
//   RegisterPayload input{
//       OSType::LINUX, ArchType::X64, TestConstants::TEST_HOSTNAME_STR,
//       TestConstants::TEST_OS_VERSION_STR, TestConstants::TEST_CURRENT_USER_STR};

//   // Act
//   const std::vector<std::uint8_t> bytes =
//       ProtocolSerializer::serializeRegisterPayload(input);
//   const RegisterPayload result = ProtocolParser::parseRegisterPayload(bytes);

//   // Assert
//   EXPECT_EQ(result.os_type, input.os_type);
//   EXPECT_EQ(result.arch, input.arch);
//   EXPECT_EQ(result.hostname, input.hostname);
//   EXPECT_EQ(result.os_version, input.os_version);
//   EXPECT_EQ(result.current_user, input.current_user);
// }

// TEST(ProtocolRoundTrip,
//      should_preserve_command_payload_through_serialize_then_parse) {
//   // Arrange
//   CommandPayload input{42, CommandType::SHELL, "whoami"};

//   // Act
//   const std::vector<std::uint8_t> bytes =
//       ProtocolSerializer::serializeCommandPayload(input);
//   const CommandPayload result = ProtocolParser::parseCommandPayload(bytes);

//   // Assert
//   EXPECT_EQ(result.id, input.id);
//   EXPECT_EQ(result.type, input.type);
//   EXPECT_EQ(result.data, input.data);
// }

// TEST(ProtocolRoundTrip,
//      should_preserve_response_payload_through_serialize_then_parse) {
//   // Arrange
// //   ResponsePayload input{9, ResponseStatus::OK, 3, 1, "chunk-data"};

//   // Act
//   const std::vector<std::uint8_t> bytes =
//       ProtocolSerializer::serializeResponsePayload(responsePayloadInput);
//   const ResponsePayload result = ProtocolParser::parseResponsePayload(bytes);

//   // Assert
//   EXPECT_EQ(result.id, responsePayloadInput.id);
//   EXPECT_EQ(result.status, responsePayloadInput.status);
//   EXPECT_EQ(result.total_chunks, responsePayloadInput.total_chunks);
//   EXPECT_EQ(result.chunk_index, responsePayloadInput.chunk_index);
//   EXPECT_EQ(result.data, responsePayloadInput.data);
// }

// TEST(ProtocolRoundTrip,
//      should_preserve_data_payload_through_serialize_then_parse) {
//   // Arrange
//   DataPayload input{DataType::KEYLOGGER, "keys"};

//   // Act
//   const std::vector<std::uint8_t> bytes =
//       ProtocolSerializer::serializeDataPayload(input);
//   const DataPayload result = ProtocolParser::parseDataPayload(bytes);

//   // Assert
//   EXPECT_EQ(result.subtype, input.subtype);
//   EXPECT_EQ(result.data, input.data);
// }

// TEST(ProtocolRoundTrip,
//      should_preserve_error_payload_through_serialize_then_parse) {
//   // Arrange
//   ErrorPayload input{ErrorType::EXECUTION_FAILED, "boom"};

//   // Act
//   const std::vector<std::uint8_t> bytes =
//       ProtocolSerializer::serializeErrorPayload(input);
//   const ErrorPayload result = ProtocolParser::parseErrorPayload(bytes);

//   // Assert
//   EXPECT_EQ(result.code, input.code);
//   EXPECT_EQ(result.message, input.message);
// }
