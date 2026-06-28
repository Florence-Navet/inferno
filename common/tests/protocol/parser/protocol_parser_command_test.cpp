#include <gtest/gtest.h>

#include "builders/frame_builder.hpp"
#include "fixtures/common.hpp"
#include "convert_endian.hpp"
#include "exception/lptf_exception.hpp"
#include "protocol/lptf_protocol.hpp"
#include "protocol/protocol_parser.hpp"

TEST(ProtocolParserCommand, should_parse_shell_command_when_input_is_valid) {
  const std::string data = "whoami";
  const std::vector<std::uint8_t> input = FrameBuilder::makeCommandPayload(
      42, static_cast<std::uint8_t>(CommandType::SHELL),
      static_cast<std::uint16_t>(data.size()), Common::bytesFromString(data));

  const CommandPayload result = ProtocolParser::parseCommandPayload(input);

  EXPECT_EQ(result.id, 42);
  EXPECT_EQ(result.type, CommandType::SHELL);
  EXPECT_EQ(result.data, data);
}

TEST(ProtocolParserCommand, should_parse_os_info_command_with_empty_data) {
  const std::vector<std::uint8_t> input = FrameBuilder::makeCommandPayload(
      7, static_cast<std::uint8_t>(CommandType::OS_INFO), 0, {});

  const CommandPayload result = ProtocolParser::parseCommandPayload(input);

  EXPECT_EQ(result.id, 7);
  EXPECT_EQ(result.type, CommandType::OS_INFO);
  EXPECT_TRUE(result.data.empty());
}

TEST(ProtocolParserCommand, should_reject_payload_shorter_than_fixed_fields) {
  const std::vector<std::uint8_t> input = {0x00, 0x2A, 0x02, 0x00};

  EXPECT_THROW(ProtocolParser::parseCommandPayload(input), InvalidSize);
}

TEST(ProtocolParserCommand,
     should_reject_when_declared_data_length_mismatches_payload_size) {
  const std::vector<std::uint8_t> input = FrameBuilder::makeCommandPayload(
      1, static_cast<std::uint8_t>(CommandType::SHELL), 10,
      Common::bytesFromString("abc"));

  EXPECT_THROW(ProtocolParser::parseCommandPayload(input), InvalidSize);
}

TEST(ProtocolParserCommand, should_reject_unknown_command_type) {
  const std::vector<std::uint8_t> input =
      FrameBuilder::makeCommandPayload(1, Common::INVALID_ENUM_VALUE, 0, {});

  EXPECT_THROW(ProtocolParser::parseCommandPayload(input), InvalidFieldValue);
}

TEST(ProtocolParserCommand, should_ignore_data_for_non_shell_command) {
  const std::vector<std::uint8_t> input = FrameBuilder::makeCommandPayload(
      1, static_cast<std::uint8_t>(CommandType::OS_INFO), 3,
      Common::bytesFromString("abc"));

  const CommandPayload command = ProtocolParser::parseCommandPayload(input);

  EXPECT_EQ(0, command.data.size());
}
