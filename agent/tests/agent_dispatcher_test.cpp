#include "agent_dispatcher.hpp"

#include <gmock/gmock.h>
#include <gtest/gtest.h>

// #include "socket/mock_socket_helpers.hpp"
#include <iostream>
#include <memory>

// #include "helpers_test.hpp"
#include "builders/frame_builder.hpp"
#include "builders/metrics_controller_test_factory.hpp"
#include "builders/os_info_builder.hpp"
#include "protocol/protocol_parser.hpp"
#include "stubs/fake_system_monitor.hpp"
#include "stubs/spy_socket.hpp"

class AgentDispatcherTest : public ::testing::Test {
 public:
  AgentDispatcherTest()
      : session(AgentSession(spy.makeUnique())),
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

TEST_F(AgentDispatcherTest,
       should_get_os_info_with_matching_id_when_requested) {
  //------- Arrange
  const std::vector<std::uint8_t> payload = FrameBuilder::makeRawCommandPayload(
      42, static_cast<std::uint8_t>(CommandType::OS_INFO), {});

  const ResponsePayload expected = {
      42, ResponseStatus::OK, 1, 0,
      ProtocolSerializer::serializeOsInfoPayload(OsInfoBuilder::create())};

  //------- Act
  dispatcher.handleFrame(
      session, FrameBuilder::makeFrame(MessageType::COMMAND, payload));

  //------- Assert
  ASSERT_GE(spy.sent.size(), static_cast<std::size_t>(LPTF_HEADER_SIZE));
  EXPECT_EQ(spy.messageType(), MessageType::RESPONSE);

  const ResponsePayload response =
      ProtocolParser::parseResponsePayload(spy.payload());

  EXPECT_EQ(response, expected);
}

TEST_F(AgentDispatcherTest, should_get_response_payload_when_requested) {
  //------- Arrange
  const std::vector<std::uint8_t> payload = FrameBuilder::makeRawCommandPayload(
      1, static_cast<std::uint8_t>(CommandType::OS_INFO), {});

  OsInfoPayload expected = FrameBuilder::makeOsInfoPayload();

  //------- Act
  dispatcher.handleFrame(
      session, FrameBuilder::makeFrame(MessageType::COMMAND, payload));

  const ResponsePayload response =
      ProtocolParser::parseResponsePayload(spy.payload());

  const OsInfoPayload osInfo =
      ProtocolParser::parseOsInfoPayload(response.data);

  //------- Assert
  EXPECT_EQ(osInfo, expected);
}

TEST_F(AgentDispatcherTest,
       should_close_session_and_send_nothing_on_disconnect) {
  //------- Act
  dispatcher.handleFrame(session,
                         FrameBuilder::makeFrame(MessageType::DISCONNECT));

  //------- Assert
  EXPECT_FALSE(session.isValid());  // socket closed
  EXPECT_TRUE(spy.nothingSent());   // no reply to DISCONNECT
}

TEST_F(AgentDispatcherTest, should_get_process_list_when_requested) {
  //------- Arrange
  const std::vector<std::uint8_t> payload = FrameBuilder::makeRawCommandPayload(
      1, static_cast<std::uint8_t>(CommandType::RUNNING_PROCESSES), {});

  const std::vector<ProcessInfo> expectedProcesses =
      ProcessBuilder::createProcessInfoList();

  const ResponsePayload expected = {
      1, ResponseStatus::OK, 1, 0,
      ProtocolSerializer::serializeProcessInfoList(expectedProcesses)};

  //------- Act
  dispatcher.handleFrame(
      session, FrameBuilder::makeFrame(MessageType::COMMAND, payload));

  //------- Assert
  ASSERT_GE(spy.sent.size(), static_cast<std::size_t>(LPTF_HEADER_SIZE));
  EXPECT_EQ(spy.messageType(), MessageType::RESPONSE);

  const ResponsePayload response =
      ProtocolParser::parseResponsePayload(spy.payload());

  EXPECT_EQ(response, expected);

  const std::vector<ProcessInfo> processes =
      ProtocolParser::parseProcessInfoList(response.data);

  ASSERT_EQ(processes.size(), 3u);

  for (std::size_t i = 0; i < processes.size(); ++i) {
    EXPECT_EQ(processes[i], expectedProcesses[i]);
  }
}

TEST_F(AgentDispatcherTest, should_activate_metrics_controller_when_requested) {
  //------- Arrange
  const std::vector<std::uint8_t> cmd = FrameBuilder::makeRawCommandPayload(
      1, static_cast<std::uint8_t>(CommandType::START_METRICS), {});
  //------- Act
  dispatcher.handleFrame(session,
                         FrameBuilder::makeFrame(MessageType::COMMAND, cmd));
  //------- Assert
  EXPECT_TRUE(controller->isActive());
}

TEST_F(AgentDispatcherTest,
       should_deactivate_metrics_controller_when_requested) {
  //------- Arrange
  controller->start(session);

  const std::vector<std::uint8_t> payload = FrameBuilder::makeRawCommandPayload(
      1, static_cast<std::uint8_t>(CommandType::STOP_METRICS), {});

  //------- Act
  dispatcher.handleFrame(
      session, FrameBuilder::makeFrame(MessageType::COMMAND, payload));

  //------- Assert
  EXPECT_FALSE(controller->isActive());
}