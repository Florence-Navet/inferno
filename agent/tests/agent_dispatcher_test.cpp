#include "agent_dispatcher.hpp"

#include <gmock/gmock.h>
#include <gtest/gtest.h>

// #include "helpers_test.hpp"
#include "builders/frame_builder.hpp"
#include "protocol/protocol_parser.hpp"
// #include "socket/mock_socket_helpers.hpp"
#include <memory>

#include "builders/frame_builder.hpp"
#include "builders/metrics_controller_test_factory.hpp"
#include "stubs/fake_system_monitor.hpp"
#include "stubs/spy_socket.hpp"

class AgentDispatcherTest : public ::testing::Test {
 public:
  AgentDispatcherTest()
      : session(makeSession(spy)),
        dispatcher(monitor),
        controller(MetricsControllerTestFactory::make(scrapperPtr)) {}

 protected:
  SpySocket spy;
  FakeSystemMonitor monitor;
  AgentSession session;
  AgentDispatcher dispatcher;
  FakeMetricsScrapper* scrapperPtr = nullptr;
  std::shared_ptr<MetricsController> controller;

  void SetUp() override { dispatcher.setMetricsController(controller); }
};

// ── AgentDispatcher tests ─────────────────────────────────────
// Three tests: happy path, disconnect handling, unknown command.
// The agent dispatcher is the mirror of the server dispatcher:
// it receives COMMANDs and sends RESPONSEs, not the other way.
// ─────────────────────────────────────────────────────────────

// ① Happy path — OS_INFO command arrives, response is sent back.
// Verifies: message type, response id matches command id, status OK,
// data is non-empty, chunk fields are correct for a single-chunk response.
TEST_F(AgentDispatcherTest,
       should_send_response_with_matching_id_on_os_info_command) {
  const std::vector<std::uint8_t> payload = FrameBuilder::makeRawCommandPayload(
      42, static_cast<std::uint8_t>(CommandType::OS_INFO), {});

  dispatcher.handleFrame(
      session, FrameBuilder::makeFrame(MessageType::COMMAND, payload));

  ASSERT_GE(spy.sent.size(), static_cast<std::size_t>(LPTF_HEADER_SIZE));
  EXPECT_EQ(spy.messageType(), MessageType::RESPONSE);

  const ResponsePayload response =
      ProtocolParser::parseResponsePayload(spy.payload());
  EXPECT_EQ(response.id, 42);  // must echo the command id
  EXPECT_EQ(response.status, ResponseStatus::OK);
  EXPECT_EQ(response.total_chunks, 1);
  EXPECT_EQ(response.chunk_index, 0);
  EXPECT_FALSE(response.data.empty());  // agent sent something
}

TEST_F(AgentDispatcherTest, should_get_response_payload) {
  const std::vector<std::uint8_t> payload = FrameBuilder::makeRawCommandPayload(
      1, static_cast<std::uint8_t>(CommandType::OS_INFO), {});

  dispatcher.handleFrame(
      session, FrameBuilder::makeFrame(MessageType::COMMAND, payload));

  static_cast<std::size_t>(LPTF_HEADER_SIZE);

  const ResponsePayload response =
      ProtocolParser::parseResponsePayload(spy.payload());

  const RegisterPayload osInfo =
      ProtocolParser::parseRegisterPayload(response.data);

  RegisterPayload expected = {
      OSType::LINUX, ArchType::X64, Protocol::TEST_HOSTNAME_STR,
      Protocol::TEST_OS_VERSION_STR, Protocol::TEST_CURRENT_USER_STR};
  EXPECT_EQ(osInfo, expected);
}

// ② DISCONNECT — server sends it, agent must close the session.
// Verifies: socket is closed (isValid() false), nothing sent back.
// The agent must NOT reply to DISCONNECT — it just closes.
TEST_F(AgentDispatcherTest,
       should_close_session_and_send_nothing_on_disconnect) {
  dispatcher.handleFrame(session,
                         FrameBuilder::makeFrame(MessageType::DISCONNECT));

  EXPECT_FALSE(session.isValid());  // socket closed
  EXPECT_TRUE(spy.nothingSent());   // no reply to DISCONNECT
}

// ③ Unknown command — agent receives a CommandType it doesn't implement.
// Verifies: ERROR frame sent back, not a crash or silent ignore.
// Covers any future CommandType added to the enum before the agent handles it.
TEST_F(AgentDispatcherTest,
       should_send_response_with_process_list_on_running_processes_command) {
  const std::vector<std::uint8_t> payload = FrameBuilder::makeRawCommandPayload(
      1, static_cast<std::uint8_t>(CommandType::RUNNING_PROCESSES), {});

  dispatcher.handleFrame(
      session, FrameBuilder::makeFrame(MessageType::COMMAND, payload));

  ASSERT_GE(spy.sent.size(), static_cast<std::size_t>(LPTF_HEADER_SIZE));
  EXPECT_EQ(spy.messageType(), MessageType::RESPONSE);

  const ResponsePayload response =
      ProtocolParser::parseResponsePayload(spy.payload());
  EXPECT_EQ(response.id, 1);
  EXPECT_EQ(response.status, ResponseStatus::OK);

  const std::vector<ProcessInfo> processes =
      ProtocolParser::parseProcessInfoList(response.data);
  ASSERT_EQ(processes.size(), 2u);
  EXPECT_EQ(processes[0].pid, 1001u);
  EXPECT_EQ(processes[0].name, "proc-a");
}

TEST_F(AgentDispatcherTest,
       should_activate_metrics_controller_on_start_metrics_command) {
  const std::vector<std::uint8_t> cmd = FrameBuilder::makeRawCommandPayload(
      1, static_cast<std::uint8_t>(CommandType::START_METRICS), {});

  dispatcher.handleFrame(session,
                         FrameBuilder::makeFrame(MessageType::COMMAND, cmd));

  EXPECT_TRUE(controller->isActive());
}

TEST_F(AgentDispatcherTest,
       should_deactivate_metrics_controller_on_stop_metrics_command) {
  controller->start(session);

  const std::vector<std::uint8_t> payload = FrameBuilder::makeRawCommandPayload(
      1, static_cast<std::uint8_t>(CommandType::STOP_METRICS), {});

  dispatcher.handleFrame(
      session, FrameBuilder::makeFrame(MessageType::COMMAND, payload));

  EXPECT_FALSE(controller->isActive());
}