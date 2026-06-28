#include <gtest/gtest.h>

#include "protocol/lptf_protocol.hpp"
#include "protocol/protocol_parser.hpp"
#include "convert_endian.hpp"
#include "exception/lptf_exception.hpp"
#include "fixtures/common.hpp"
#include "builders/frame_builder.hpp"


TEST(ProtocolParserError, should_parse_error_payload_when_input_is_valid) {
  const std::string message = "invalid format";
  const std::vector<std::uint8_t> input =
      FrameBuilder::makeErrorPayload(static_cast<std::uint8_t>(ErrorType::INVALID_FORMAT),
                       static_cast<std::uint16_t>(message.size()),
                       Common::bytesFromString(message));

  const ErrorPayload result = ProtocolParser::parseErrorPayload(input);

  EXPECT_EQ(result.code, ErrorType::INVALID_FORMAT);
  EXPECT_EQ(result.message, message);
}

TEST(ProtocolParserError, should_reject_payload_shorter_than_fixed_fields) {
  const std::vector<std::uint8_t> input = {0x00, 0x00};

  EXPECT_THROW(ProtocolParser::parseErrorPayload(input), InvalidSize);
}

TEST(ProtocolParserError, should_reject_unknown_error_code) {
  const std::string message = "whatever message";
  const std::vector<std::uint8_t> input =
      FrameBuilder::makeErrorPayload(Common::INVALID_ENUM_VALUE, message.size(),
                       Common::bytesFromString(message));

  EXPECT_THROW(ProtocolParser::parseErrorPayload(input), InvalidFieldValue);
}

TEST(ProtocolParserError,
     should_reject_when_declared_message_length_mismatches_payload_size) {
  const std::vector<std::uint8_t> input =
      FrameBuilder::makeErrorPayload(static_cast<std::uint8_t>(ErrorType::EXECUTION_FAILED),
                       10, Common::bytesFromString("abc"));

  EXPECT_THROW(ProtocolParser::parseErrorPayload(input), InvalidSize);
}

TEST(ProtocolParserError, should_reject_when_declared_message_length_is_null)
{
  const std::vector<std::uint8_t> input = FrameBuilder::makeErrorPayload(
      static_cast<std::uint8_t>(ErrorType::UNKNOWN_TYPE), 0, {});

  EXPECT_THROW(ProtocolParser::parseErrorPayload(input), InvalidSize);
}
