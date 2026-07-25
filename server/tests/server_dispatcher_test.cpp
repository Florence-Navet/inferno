#include "server_dispatcher.hpp"

#include <gtest/gtest.h>

#include "agent_connection.hpp"
#include "builders/frame_builder.hpp"
#include "codec/protocol_parser.hpp"
#include "fixtures/common.hpp"
#include "session_manager.hpp"
#include "stubs/spy_socket.hpp"

class ServerDispatcherTest : public ::testing::Test {
 protected:
  SessionManager manager;
  ServerDispatcher dispatcher{manager};

  // Helper: Create an agent through the manager (mirrors Reactor::onNewConnection)
  AgentConnection& createAgent(int fd, SpySocket& spy) {
    manager.addAgent(fd, spy.makeUnique());
    return manager.getAgent(fd);
  }

  // Helper: Create and register a dashboard through the manager
  AgentConnection& createDashboard(int fd, SpySocket& spy) {
    manager.addAgent(fd, spy.makeUnique());
    manager.setDashboardFd(fd);
    AgentConnection& dashboard = manager.getDashboard();
    // Dashboard doesn't call recordAgentTarget (it's not a routing target)
    return dashboard;
  }
};

// ════════════════════════════════════════════════════════════════════════════
// Basic Agent Registration Tests
// ════════════════════════════════════════════════════════════════════════════

TEST_F(ServerDispatcherTest, should_register_session_on_register) {
  // Arrange
  SpySocket agentSpy;
  AgentConnection& agent = createAgent(101, agentSpy);

  // Act
  dispatcher.handleFrame(
      agent, FrameBuilder::makeFrame(MessageType::REGISTER,
                                     FrameBuilder::makeRawOsInfoPayload()));

  // Assert
  EXPECT_TRUE(agent.getIsRegistered());
  EXPECT_EQ(agent.getAgentInfo().hostname, Protocol::TEST_HOSTNAME_STR);
}

TEST_F(ServerDispatcherTest,
       should_send_error_when_unknown_message_type_received) {
  // Arrange
  SpySocket agentSpy;
  AgentConnection& agent = createAgent(101, agentSpy);

  // Act
  // COMMAND is server→agent — receiving it is a protocol violation
  dispatcher.handleFrame(agent,
                         FrameBuilder::makeFrame(MessageType::COMMAND));

  // Assert
  ASSERT_GE(agentSpy.sent.size(), static_cast<std::size_t>(LPTF_HEADER_SIZE));
  EXPECT_EQ(agentSpy.messageType(), MessageType::ERROR);
}

TEST_F(ServerDispatcherTest,
       should_increment_command_id_across_successive_commands) {
  // Arrange
  SpySocket agentSpy;
  AgentConnection& agent = createAgent(101, agentSpy);
  std::vector<std::uint16_t> ids;

  // Act
  for (int i = 0; i < 3; ++i) {
    dispatcher.sendCommand(agent, CommandType::OS_INFO, "");

    // feed sent bytes into receive buffer
    agent.appendToBuffer(agentSpy.sent);

    // clear spy so each iteration is isolated
    agentSpy.sent.clear();

    // extract frames via real RX pipeline
    while (auto frame = agent.tryExtractFrame()) {
      CommandPayload cmd = ProtocolParser::parseCommandPayload(frame->payload);
      ids.push_back(cmd.id);
    }
  }

  // Assert
  ASSERT_EQ(ids.size(), 3);
  EXPECT_EQ(ids[0], 0);
  EXPECT_EQ(ids[1], 1);
  EXPECT_EQ(ids[2], 2);
}

// ════════════════════════════════════════════════════════════════════════════
// Dashboard Registration Tests
// ════════════════════════════════════════════════════════════════════════════

TEST_F(ServerDispatcherTest, should_register_dashboard_on_dashboard_register) {
  // Arrange
  SpySocket dashboardSpy;
  AgentConnection& dashboard = createDashboard(100, dashboardSpy);

  // Act
  dispatcher.handleFrame(
      dashboard, FrameBuilder::makeFrame(MessageType::DASHBOARD_REGISTER,
                                         FrameBuilder::makeRawOsInfoPayload()));

  // Assert
  EXPECT_TRUE(dashboard.getIsRegistered());
  EXPECT_EQ(dashboard.getAgentInfo().hostname, Protocol::TEST_HOSTNAME_STR);
}

TEST_F(ServerDispatcherTest,
       should_send_agents_list_when_dashboard_registers_empty) {
  // Arrange
  SpySocket dashboardSpy;
  AgentConnection& dashboard = createDashboard(100, dashboardSpy);

  // Act - Dashboard registers with no agents connected
  dispatcher.handleFrame(
      dashboard, FrameBuilder::makeFrame(MessageType::DASHBOARD_REGISTER,
                                         FrameBuilder::makeRawOsInfoPayload()));

  // Assert - A DATA frame should be sent
  ASSERT_GE(dashboardSpy.sent.size(), static_cast<std::size_t>(LPTF_HEADER_SIZE));
  EXPECT_EQ(dashboardSpy.messageType(), MessageType::DATA);

  // Verify payload is AGENTS subtype with empty data
  const DataPayload data = ProtocolParser::parseDataPayload(dashboardSpy.payload());
  EXPECT_EQ(data.subtype, DataType::AGENTS);
  EXPECT_EQ(data.data.size(), 0);  // No agents connected
}

// ════════════════════════════════════════════════════════════════════════════
// Agent Registration with Connected Dashboard Tests
// ════════════════════════════════════════════════════════════════════════════

TEST_F(ServerDispatcherTest,
       should_notify_dashboard_when_agent_registers_after_dashboard) {
  // Arrange: Dashboard already connected
  SpySocket dashboardSpy;
  AgentConnection& dashboard = createDashboard(100, dashboardSpy);
  dashboard.setId("dashboard-id");

  // Clear initial AGENTS list message from dashboard registration
  dashboardSpy.sent.clear();

  // Act: Agent registers
  SpySocket agentSpy;
  AgentConnection& agent = createAgent(101, agentSpy);

  dispatcher.handleFrame(
      agent, FrameBuilder::makeFrame(MessageType::REGISTER,
                                     FrameBuilder::makeRawOsInfoPayload()));

  // Assert: Dashboard received REGISTRATION notification
  ASSERT_GE(dashboardSpy.sent.size(),
            static_cast<std::size_t>(LPTF_HEADER_SIZE));
  EXPECT_EQ(dashboardSpy.messageType(), MessageType::DATA);

  // Verify it's a REGISTRATION subtype
  const DataPayload data = ProtocolParser::parseDataPayload(dashboardSpy.payload());
  EXPECT_EQ(data.subtype, DataType::REGISTRATION);
}

TEST_F(ServerDispatcherTest,
       should_not_crash_when_agent_registers_without_dashboard) {
  // Arrange
  SpySocket agentSpy;
  AgentConnection& agent = createAgent(101, agentSpy);

  // Act - Agent registers with no dashboard connected
  dispatcher.handleFrame(
      agent, FrameBuilder::makeFrame(MessageType::REGISTER,
                                     FrameBuilder::makeRawOsInfoPayload()));

  // Assert - Should register successfully even without dashboard
  EXPECT_TRUE(agent.getIsRegistered());
  EXPECT_EQ(agent.getAgentInfo().hostname, Protocol::TEST_HOSTNAME_STR);
}

TEST_F(ServerDispatcherTest,
       should_send_agents_list_when_dashboard_registers_with_agents) {
  // Arrange: Setup two agents first
  SpySocket agent1Spy;
  AgentConnection& agent1 = createAgent(101, agent1Spy);
  agent1.setAgentInfo(FrameBuilder::makeOsInfoPayload());
  agent1.setIsRegisered();
  agent1.setId("agent1-id");

  SpySocket agent2Spy;
  AgentConnection& agent2 = createAgent(102, agent2Spy);
  agent2.setAgentInfo(FrameBuilder::makeOsInfoPayload());
  agent2.setIsRegisered();
  agent2.setId("agent2-id");

  // Act: Dashboard registers
  SpySocket dashboardSpy;
  AgentConnection& dashboard = createDashboard(100, dashboardSpy);

  dispatcher.handleFrame(
      dashboard, FrameBuilder::makeFrame(MessageType::DASHBOARD_REGISTER,
                                         FrameBuilder::makeRawOsInfoPayload()));

  // Assert: Dashboard received AGENTS with 2 agents
  ASSERT_GE(dashboardSpy.sent.size(), static_cast<std::size_t>(LPTF_HEADER_SIZE));
  EXPECT_EQ(dashboardSpy.messageType(), MessageType::DATA);

  const DataPayload data = ProtocolParser::parseDataPayload(dashboardSpy.payload());
  EXPECT_EQ(data.subtype, DataType::AGENTS);
  EXPECT_GT(data.data.size(), 0);  // Has agent data
}