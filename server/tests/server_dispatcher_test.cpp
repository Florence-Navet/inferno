#include "server_dispatcher.hpp"

#include <gtest/gtest.h>

// #include "helpers_test.hpp"
#include "builders/frame_builder.hpp"
#include "protocol/protocol_parser.hpp"
#include "stubs/spy_socket.hpp"
// #include "test_constants.hpp"
#include "fixtures/common.hpp"

// ServerDispatcher — 3 tests covering the only stable invariants.
// SpySocket accumulates all sent bytes; we parse them back with
// ProtocolParser to check wire correctness without GMock matchers.

TEST(ServerDispatcher,
     should_register_session_and_send_os_info_command_on_register) {
  SpySocket spy;
  AgentSession session = makeSession(spy);
  ServerDispatcher dispatcher;

  dispatcher.handleFrame(
      session, FrameBuilder::makeFrame(MessageType::REGISTER,
                                       FrameBuilder::makeRegisterPayload()));

  EXPECT_TRUE(session.getIsRegistered());
  EXPECT_EQ(session.getAgentInfo().hostname, Protocol::TEST_HOSTNAME_STR);

  ASSERT_GE(spy.sent.size(), static_cast<std::size_t>(LPTF_HEADER_SIZE));
  EXPECT_EQ(spy.messageType(), MessageType::COMMAND);

  const CommandPayload cmd = ProtocolParser::parseCommandPayload(spy.payload());
  EXPECT_EQ(cmd.type, CommandType::OS_INFO);
  EXPECT_EQ(cmd.id, 0);
}

TEST(ServerDispatcher, should_send_error_when_unknown_message_type_received) {
  SpySocket spy;
  AgentSession session = makeSession(spy);
  ServerDispatcher dispatcher;

  // COMMAND is server→agent — receiving it is a protocol violation
  dispatcher.handleFrame(session,
                         FrameBuilder::makeFrame(MessageType::COMMAND));

  ASSERT_GE(spy.sent.size(), static_cast<std::size_t>(LPTF_HEADER_SIZE));
  EXPECT_EQ(spy.messageType(), MessageType::ERROR);
}

TEST(ServerDispatcher,
     should_increment_command_id_across_successive_register_frames) {
  ServerDispatcher dispatcher;
  std::vector<std::uint16_t> ids;

  for (int i = 0; i < 3; ++i) {
    SpySocket spy;
    AgentSession session = makeSession(spy);
    dispatcher.handleFrame(
        session, FrameBuilder::makeFrame(MessageType::REGISTER,
                                         FrameBuilder::makeRegisterPayload()));

    ASSERT_GE(spy.sent.size(), static_cast<std::size_t>(LPTF_HEADER_SIZE));
    const CommandPayload cmd =
        ProtocolParser::parseCommandPayload(spy.payload());
    ids.push_back(cmd.id);
  }

  EXPECT_EQ(ids[0], 0);
  EXPECT_EQ(ids[1], 1);
  EXPECT_EQ(ids[2], 2);
}
