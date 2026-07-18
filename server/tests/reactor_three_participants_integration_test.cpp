
#include <gtest/gtest.h>

#include <future>
#include <optional>
#include <thread>

#include "agent_connection.hpp"
#include "builders/frame_builder.hpp"
#include "codec/protocol_parser.hpp"
#include "fixtures/common.hpp"
#include "fixtures/ports.hpp"
#include "poller/epoller.hpp"
#include "protocol/lptf_protocol.hpp"
#include "reactor.hpp"
#include "server_dispatcher.hpp"
#include "session_manager.hpp"
#include "socket/socket_factory.hpp"
#include "tcp_server.hpp"
// ... other includes
class ThreeParticipantIntegrationTest : public ::testing::Test {
 protected:
  Epoller epoller;
  SessionManager manager;
  ServerDispatcher dispatcher{manager};

  const std::uint16_t TEST_PORT = Ports::Reactor::THREE_PARTICIPANT_PORT;

  std::thread startServer(TcpServer& server, Reactor& reactor,
                          std::promise<void>& serverReady) {
    return std::thread([&] {
      server.start();
      server.setNonBlocking();
      serverReady.set_value();
      reactor.run();
    });
  }

  std::thread connectAndListenDashboard(
      std::uint16_t port, std::promise<void>& dashboardReady,
      std::promise<Frame>& agentInfoReceived) {
    return std::thread([port, &dashboardReady, &agentInfoReceived] {
      try {
        auto socket = SocketFactory::createTCP();
        if (!socket->connect(Common::SERVER_HOST, port)) {
          throw std::runtime_error("dashboard connection failed");
        }

        OsInfoPayload info = FrameBuilder::makeOsInfoPayload();
        std::vector<std::uint8_t> payload =
            ProtocolSerializer::serializeOsInfoPayload(info);

        Frame registerFrame;
        registerFrame.header = ProtocolHelper::createHeader(
            MessageType::DASHBOARD_REGISTER, payload);
        registerFrame.payload = payload;
        auto serialized = ProtocolSerializer::serializeFrame(registerFrame);
        socket->send(serialized);

        dashboardReady.set_value();

        // Keep listening for agent data until received
        // Keep listening for agent data until received
        AgentConnection dashboardConn(std::move(socket));
        for (int i = 0; i < 100; i++) {  // ~1 second total
          std::this_thread::sleep_for(std::chrono::milliseconds(10));
          SocketResult result = dashboardConn.receiveIntoBuffer();
          if (result.ok() && result.bytesTransferred > 0) {
            std::optional<Frame> frame = dashboardConn.tryExtractFrame();
            if (frame.has_value() && frame->header.type == MessageType::DATA) {
              DataPayload data =
                  ProtocolParser::parseDataPayload(frame->payload);
              // Skip AGENTS messages, wait for REGISTRATION
              if (data.subtype == DataType::REGISTRATION) {
                try {
                  agentInfoReceived.set_value(frame.value());
                  return;  // Exit after receiving the right message
                } catch (...) {
                }
              }
            }
          }
        }
      } catch (const std::exception& e) {
        try {
          dashboardReady.set_exception(std::current_exception());
        } catch (...) {
        }
      }
    });
  }

  std::thread connectAgent(std::uint16_t port, const std::string& hostname,
                           std::promise<void>& agentReady) {
    return std::thread([port, hostname, &agentReady] {
      try {
        auto socket = SocketFactory::createTCP();
        if (!socket->connect(Common::SERVER_HOST, port)) {
          throw std::runtime_error("agent connection failed");
        }

        OsInfoPayload agentInfo = FrameBuilder::makeOsInfoPayload(
            OSType::LINUX, ArchType::X64, hostname);
        auto payload = ProtocolSerializer::serializeOsInfoPayload(agentInfo);
        Frame registerFrame;
        registerFrame.header =
            ProtocolHelper::createHeader(MessageType::REGISTER, payload);
        registerFrame.payload = payload;
        auto serialized = ProtocolSerializer::serializeFrame(registerFrame);
        socket->send(serialized);

        agentReady.set_value();
        // Thread exits naturally after setting value
      } catch (const std::exception& e) {
        try {
          agentReady.set_exception(std::current_exception());
        } catch (...) {
        }
      }
    });
  }
};

TEST_F(ThreeParticipantIntegrationTest,
       Dashboard_Receives_Agent_Registration_Over_Server) {
  TcpServer server(TEST_PORT);
  Reactor reactor(server, dispatcher, epoller, manager);

  std::promise<void> serverReady, dashboardReady, agentReady;
  std::promise<Frame> agentInfoReceived;

  auto serverThread = startServer(server, reactor, serverReady);
  serverReady.get_future().get();
  std::this_thread::sleep_for(std::chrono::milliseconds(50));

  auto dashboardThread =
      connectAndListenDashboard(TEST_PORT, dashboardReady, agentInfoReceived);
  dashboardReady.get_future().get();
  std::this_thread::sleep_for(std::chrono::milliseconds(50));

  auto agentThread = connectAgent(TEST_PORT, "production-host-1", agentReady);
  agentReady.get_future().get();
  std::this_thread::sleep_for(std::chrono::milliseconds(50));

  // Wait for dashboard to receive agent data (dashboard thread stays alive
  // listening)
  auto future = agentInfoReceived.get_future();
  auto receiveStatus = future.wait_for(std::chrono::seconds(2));

  reactor.stop();
  serverThread.join();
  dashboardThread.join();
  agentThread.join();

  EXPECT_EQ(receiveStatus, std::future_status::ready);
  if (receiveStatus == std::future_status::ready) {
    Frame receivedFrame = future.get();  // ← Reuse the same future
    EXPECT_EQ(receivedFrame.header.type, MessageType::DATA);

    DataPayload data = ProtocolParser::parseDataPayload(receivedFrame.payload);
    EXPECT_EQ(data.subtype, DataType::REGISTRATION);

    OsInfoPayload agentInfo = ProtocolParser::parseOsInfoPayload(data.data);
    EXPECT_EQ(agentInfo.hostname, "production-host-1");
  }
}