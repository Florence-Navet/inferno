#include <gtest/gtest.h>

#include <string>
#include <vector>

#include "builders/frame_builder.hpp"
#include "fixtures/protocol.hpp"
#include "protocol/lptf_protocol.hpp"
#include "protocol/protocol_parser.hpp"
#include "protocol/protocol_serializer.hpp"

TEST(ProtocolRoundTrip,
     should_preserve_register_payload_through_serialize_then_parse) {

  OsInfoPayload input = FrameBuilder::makeOsInfoPayload();

  const std::vector<std::uint8_t> bytes =
      ProtocolSerializer::serializeOsInfoPayload(input);
  const OsInfoPayload result = ProtocolParser::parseOsInfoPayload(bytes);

  EXPECT_EQ(result, input);

//   EXPECT_EQ(result.os_type, input.os_type);
//   EXPECT_EQ(result.arch, input.arch);
//   EXPECT_EQ(result.hostname, input.hostname);
//   EXPECT_EQ(result.os_version, input.os_version);
//   EXPECT_EQ(result.current_user, input.current_user);
}

TEST(ProtocolRoundTrip,
     should_preserve_command_payload_through_serialize_then_parse) {
  CommandPayload input{42, CommandType::SHELL, "whoami"};

  const std::vector<std::uint8_t> bytes =
      ProtocolSerializer::serializeCommandPayload(input);
  const CommandPayload result = ProtocolParser::parseCommandPayload(bytes);

  EXPECT_EQ(result.id, input.id);
  EXPECT_EQ(result.type, input.type);
  EXPECT_EQ(result.data, input.data);
}

TEST(ProtocolRoundTrip,
     should_preserve_response_payload_through_serialize_then_parse) {
  ResponsePayload input;
  input.id = 9;
  input.status = ResponseStatus::OK;
  input.total_chunks = 3;
  input.chunk_index = 1;
  input.data = {'c', 'h', 'u', 'n', 'k'};

  const std::vector<std::uint8_t> bytes =
      ProtocolSerializer::serializeResponsePayload(input);
  const ResponsePayload result = ProtocolParser::parseResponsePayload(bytes);

  EXPECT_EQ(result.id, input.id);
  EXPECT_EQ(result.status, input.status);
  EXPECT_EQ(result.total_chunks, input.total_chunks);
  EXPECT_EQ(result.chunk_index, input.chunk_index);
  EXPECT_EQ(result.data, input.data);
}

TEST(ProtocolRoundTrip,
     should_preserve_data_payload_through_serialize_then_parse) {
  DataPayload input{DataType::METRICS_SAMPLE, {'k', 'e', 'y', 's'}};

  const std::vector<std::uint8_t> bytes =
      ProtocolSerializer::serializeDataPayload(input);
  const DataPayload result = ProtocolParser::parseDataPayload(bytes);

  EXPECT_EQ(result.subtype, input.subtype);
  EXPECT_EQ(result.data, input.data);
}

TEST(ProtocolRoundTrip,
     should_preserve_error_payload_through_serialize_then_parse) {
  ErrorPayload input{ErrorType::EXECUTION_FAILED, "boom"};

  const std::vector<std::uint8_t> bytes =
      ProtocolSerializer::serializeErrorPayload(input);
  const ErrorPayload result = ProtocolParser::parseErrorPayload(bytes);

  EXPECT_EQ(result.code, input.code);
  EXPECT_EQ(result.message, input.message);
}