#include <gtest/gtest.h>

#include <string>
#include <vector>

#include "builders/frame_builder.hpp"
#include "fixtures/protocol.hpp"
#include "protocol/lptf_protocol.hpp"
#include "codec/protocol_parser.hpp"
#include "codec/protocol_serializer.hpp"

TEST(ProtocolRoundTrip,
     should_preserve_os_info_payload_through_serialize_then_parse) {
  OsInfoPayload input = FrameBuilder::makeOsInfoPayload();

  const std::vector<std::uint8_t> bytes =
      ProtocolSerializer::serializeOsInfoPayload(input);
  const OsInfoPayload result = ProtocolParser::parseOsInfoPayload(bytes);

  EXPECT_EQ(result, input);
}

TEST(ProtocolRoundTrip,
     should_preserve_command_payload_through_serialize_then_parse) {
  CommandPayload input = FrameBuilder::makeCommandPayload();

  const std::vector<std::uint8_t> bytes =
      ProtocolSerializer::serializeCommandPayload(input);
  const CommandPayload result = ProtocolParser::parseCommandPayload(bytes);

  EXPECT_EQ(result, input);
}

TEST(ProtocolRoundTrip,
     should_preserve_response_payload_through_serialize_then_parse) {
  ResponsePayload input = FrameBuilder::makeResponsePayload();

  const std::vector<std::uint8_t> bytes =
      ProtocolSerializer::serializeResponsePayload(input);
  const ResponsePayload result = ProtocolParser::parseResponsePayload(bytes);
  EXPECT_EQ(result, input);
}

TEST(ProtocolRoundTrip,
     should_preserve_data_payload_through_serialize_then_parse) {
  DataPayload input{DataType::METRICS_SAMPLE, {'k', 'e', 'y', 's'}};

  const std::vector<std::uint8_t> bytes =
      ProtocolSerializer::serializeDataPayload(input);
  const DataPayload result = ProtocolParser::parseDataPayload(bytes);

  EXPECT_EQ(result, input);
}

TEST(ProtocolRoundTrip,
     should_preserve_error_payload_through_serialize_then_parse) {
  ErrorPayload input{ErrorType::EXECUTION_FAILED, "boom"};

  const std::vector<std::uint8_t> bytes =
      ProtocolSerializer::serializeErrorPayload(input);
  const ErrorPayload result = ProtocolParser::parseErrorPayload(bytes);
  EXPECT_EQ(result, input);
}

TEST(ProtocolRoundTrip, should_preserve_header_through_serialize_then_parse) {
  LptfHeader input;
  input.identifier = LPTF_IDENTIFIER;
  input.type = MessageType::COMMAND;

  const std::vector<std::uint8_t> bytes =
      ProtocolSerializer::serializeHeader(input);
  const LptfHeader result = ProtocolParser::parseHeader(bytes);
  EXPECT_EQ(result, input);
}