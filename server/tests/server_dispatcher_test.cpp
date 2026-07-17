#include "server_dispatcher.hpp"

#include <gtest/gtest.h>

#include "agent_connection.hpp"
#include "builders/frame_builder.hpp"
#include "codec/protocol_parser.hpp"
#include "fixtures/common.hpp"
#include "stubs/spy_socket.hpp"
#include "session_manager.hpp"
TEST(ServerDispatcher, should_register_session_on_register) {
  SpySocket spy;
  SessionManager manager;
  // AgentSession session = makeSession(spy);
  AgentConnection session = AgentConnection(spy.makeUnique());
  ServerDispatcher dispatcher(manager);

  dispatcher.handleFrame(
      session, FrameBuilder::makeFrame(MessageType::REGISTER,
                                       FrameBuilder::makeRawOsInfoPayload()));

  EXPECT_TRUE(session.getIsRegistered());
  EXPECT_EQ(session.getAgentInfo().hostname, Protocol::TEST_HOSTNAME_STR);
  // EXPECT_EQ(session.getAgentInfo(), FrameBuilder::makeRawOsInfoPayload())
}

TEST(ServerDispatcher, should_send_error_when_unknown_message_type_received) {
  SpySocket spy;
  // AgentSession session = makeSession(spy);
  AgentConnection session = AgentConnection(spy.makeUnique());
  SessionManager manager;
  ServerDispatcher dispatcher(manager);

  // COMMAND is server→agent — receiving it is a protocol violation
  dispatcher.handleFrame(session,
                         FrameBuilder::makeFrame(MessageType::COMMAND));

  ASSERT_GE(spy.sent.size(), static_cast<std::size_t>(LPTF_HEADER_SIZE));
  EXPECT_EQ(spy.messageType(), MessageType::ERROR);
}

TEST(ServerDispatcher,
     should_increment_command_id_across_successive_register_frames) {
  SpySocket spy;
  SessionManager manager;
  // AgentSession session = makeSession(spy);
  AgentConnection session = AgentConnection(spy.makeUnique());
  ServerDispatcher dispatcher(manager);

  std::vector<std::uint16_t> ids;

  for (int i = 0; i < 3; ++i) {
    dispatcher.sendCommand(session, CommandType::OS_INFO, "");

    // feed sent bytes into receive buffer
    session.appendToBuffer(spy.sent);

    // clear spy so each iteration is isolated
    spy.sent.clear();

    // extract frames via real RX pipeline
    while (auto frame = session.tryExtractFrame()) {
      CommandPayload cmd = ProtocolParser::parseCommandPayload(frame->payload);

      ids.push_back(cmd.id);
    }
  }

  ASSERT_EQ(ids.size(), 3);
  EXPECT_EQ(ids[0], 0);
  EXPECT_EQ(ids[1], 1);
  EXPECT_EQ(ids[2], 2);
}
