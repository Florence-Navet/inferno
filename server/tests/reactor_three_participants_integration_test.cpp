
// #include <gtest/gtest.h>

// #include <future>
// #include <optional>
// #include <thread>

// #include "agent_connection.hpp"
// #include "builders/frame_builder.hpp"
// #include "codec/protocol_parser.hpp"
// #include "fixtures/common.hpp"
// #include "fixtures/ports.hpp"
// #include "poller/epoller.hpp"
// #include "protocol/lptf_protocol.hpp"
// #include "reactor.hpp"
// #include "server_dispatcher.hpp"
// #include "session_manager.hpp"
// #include "socket/socket_factory.hpp"
// #include "tcp_server.hpp"
// // ... other includes

// class ThreeParticipantIntegrationTest : public ::testing::Test {
//  protected:
//   Epoller epoller;
//   SessionManager manager;
//   ServerDispatcher dispatcher{manager};

//   const std::uint16_t TEST_PORT = Ports::Reactor::THREE_PARTICIPANT_PORT;

//   // ─────────────────────────────────────────────────────────────────────
//   // Helper: Start server reactor with readiness signal
//   // ─────────────────────────────────────────────────────────────────────
//   std::thread startServer(TcpServer& server, Reactor& reactor,
//                           std::promise<void>& serverReady) {
//     return std::thread([&] {
//       server.start();
//       server.setNonBlocking();
//       serverReady.set_value();  // Signal: reactor loop is starting
//       reactor.run();
//     });
//   }

//   // ─────────────────────────────────────────────────────────────────────
//   // Helper: Dashboard connects and sends REGISTER, waits for confirmation
//   // ─────────────────────────────────────────────────────────────────────
//   std::thread connectAndListenDashboard(
//       std::uint16_t port, std::promise<void>& dashboardReady,
//       std::promise<Frame>& agentInfoReceived) {
//     return std::thread([port, &dashboardReady, &agentInfoReceived] {
//       auto socket = SocketFactory::createTCP();
//         // EXPECT_TRUE(socket->connect(Common::SERVER_HOST, port));
//       if (!socket->connect(Common::SERVER_HOST, port)) {
//         dashboardReady.set_exception(std::make_exception_ptr(
//             std::runtime_error("dashboard connection failed")));
//         return;
//       }

//       OsInfoPayload info = FrameBuilder::makeOsInfoPayload();
//       std::vector<std::uint8_t> payload =
//           ProtocolSerializer::serializeOsInfoPayload(info);

//       Frame registerFrame;
//       registerFrame.header = ProtocolHelper::createHeader(
//           MessageType::DASHBOARD_REGISTER, payload);
//       registerFrame.payload = payload;
//       auto serialized = ProtocolSerializer::serializeFrame(registerFrame);
//         // EXPECT_TRUE(socket->send(serialized).ok());
//       socket->send(serialized).ok();

//       dashboardReady.set_value();  // <-- ONLY set here, once

//       AgentConnection dashboardConn(std::move(socket));
//       SocketResult result = dashboardConn.receiveIntoBuffer();
//       if (result.ok()) {
//         std::optional<Frame> frame = dashboardConn.tryExtractFrame();
//         if (frame.has_value() && frame->header.type == MessageType::DATA) {
//           agentInfoReceived.set_value(frame.value());
//         }
//       }
//     });
//   }

//   // ─────────────────────────────────────────────────────────────────────
//   // Helper: Agent connects and sends REGISTER
//   // ─────────────────────────────────────────────────────────────────────
//   std::thread connectAgent(std::uint16_t port, const std::string& hostname,
//                            std::promise<void>& agentReady) {
//     return std::thread([port, hostname, &agentReady] {
//       auto socket = SocketFactory::createTCP();
//         // EXPECT_TRUE(socket->connect(Common::SERVER_HOST, port));
//       socket->connect(Common::SERVER_HOST, port);

//       OsInfoPayload agentInfo = FrameBuilder::makeOsInfoPayload(
//           OSType::LINUX, ArchType::X64, hostname);
//       auto payload = ProtocolSerializer::serializeOsInfoPayload(agentInfo);
//       Frame registerFrame;
//       registerFrame.header =
//           ProtocolHelper::createHeader(MessageType::REGISTER, payload);
//       registerFrame.payload = payload;
//       auto serialized = ProtocolSerializer::serializeFrame(registerFrame);
//         // EXPECT_TRUE(socket->send(serialized).ok());
//       socket->send(serialized).ok();

//       agentReady.set_value();  // Signal: agent connected and registered

//       // Keep socket alive (agent stays connected)
//       std::this_thread::sleep_for(std::chrono::seconds(5));
//     });
//   }
// };

// // ═══════════════════════════════════════════════════════════════════════════
// // TEST 1: Three-way handshake
// // ═══════════════════════════════════════════════════════════════════════════
// TEST_F(ThreeParticipantIntegrationTest,
//        Dashboard_Receives_Agent_Registration_Over_Server) {
//   TcpServer server(TEST_PORT);
//   Reactor reactor(server, dispatcher, epoller, manager);

//   std::promise<void> serverReady;
//   std::promise<void> dashboardReady;
//   std::promise<void> agentReady;
//   std::promise<Frame> agentInfoReceived;

//   auto serverThread = startServer(server, reactor, serverReady);
//   serverReady.get_future().wait();
//   std::this_thread::sleep_for(std::chrono::milliseconds(50));

//   auto dashboardThread =
//       connectAndListenDashboard(TEST_PORT, dashboardReady, agentInfoReceived);
//   dashboardReady.get_future().wait();
//   std::this_thread::sleep_for(std::chrono::milliseconds(50));

//   auto agentThread = connectAgent(TEST_PORT, "production-host-1", agentReady);
//   agentReady.get_future().wait();
//   std::this_thread::sleep_for(std::chrono::milliseconds(50));

//   // NOW ASSERT
//   auto receiveStatus =
//       agentInfoReceived.get_future().wait_for(std::chrono::seconds(1));

//   EXPECT_EQ(receiveStatus, std::future_status::ready);

//   if (receiveStatus == std::future_status::ready) {
//     Frame receivedFrame = agentInfoReceived.get_future().get();
//     EXPECT_EQ(receivedFrame.header.type, MessageType::DATA);

//     DataPayload data = ProtocolParser::parseDataPayload(receivedFrame.payload);
//     EXPECT_EQ(data.subtype, DataType::REGISTRATION);

//     OsInfoPayload agentInfo = ProtocolParser::parseOsInfoPayload(data.data);
//     EXPECT_EQ(agentInfo.hostname, "production-host-1");
//   }

//   reactor.stop();
//   serverThread.join();
//   dashboardThread.join();
//   agentThread.join();
// }