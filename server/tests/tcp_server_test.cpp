#include "tcp_server.hpp"

#include <gtest/gtest.h>
#include <sys/socket.h>
#include <unistd.h>

#include <future>
#include <optional>
#include <thread>

#include "agent_session.hpp"
#include "protocol/protocol_helper.hpp"
#include "protocol/protocol_parser.hpp"
#include "protocol/protocol_serializer.hpp"
#include "socket/i_socket.hpp"
#include "socket/socket_factory.hpp"
#include "test_constants.hpp"

namespace {

std::vector<std::uint8_t> buildRawFrame(
    MessageType type, const std::vector<std::uint8_t>& payload) {
  const LptfHeader header = ProtocolHelper::createHeader(type, payload);
  std::vector<std::uint8_t> bytes =
      ProtocolSerializer::serializeHeader(header);
  bytes.insert(bytes.end(), payload.begin(), payload.end());
  return bytes;
}

}  // namespace

// All integration tests connect using SocketFactory::createTCP() — the same
// path the real agent uses — rather than raw syscall helpers.

// ── Unit tests (no network) ───────────────────────────────────

TEST(TcpServerUnit,
     should_return_false_when_start_called_on_already_used_port) {
  // Occupy the port first using a raw socket (intentional: we're testing
  // that TcpServer correctly detects a port conflict, not the agent path)
  sockaddr_in address{};
  address.sin_family = AF_INET;
  address.sin_addr.s_addr = INADDR_ANY;
  address.sin_port = htons(TestConstants::TCP_SERVER_OCCUPIED_PORT);
  int occupier = ::socket(AF_INET, SOCK_STREAM, 0);
  ASSERT_NE(occupier, -1);
  ASSERT_EQ(
      ::bind(occupier, reinterpret_cast<sockaddr*>(&address), sizeof(address)),
      0);

  TcpServer server(TestConstants::TCP_SERVER_OCCUPIED_PORT);
  EXPECT_FALSE(server.start());

  ::close(occupier);
}

TEST(TcpServerUnit,
     should_return_nullptr_when_acceptAgent_called_before_start) {
  TcpServer server(TestConstants::TCP_SERVER_NOT_STARTED_PORT);
  EXPECT_EQ(server.acceptAgent(), nullptr);
}

// ── Integration tests (real loopback) ────────────────────────

TEST(TcpServerIntegration,
     should_return_true_when_start_is_called_on_available_port) {
  TcpServer server(TestConstants::TCP_SERVER_AVAILABLE_PORT);
  EXPECT_TRUE(server.start());
}

TEST(TcpServerIntegration,
     should_return_false_when_start_is_called_a_second_time) {
  TcpServer server(TestConstants::TCP_SERVER_DOUBLE_START_PORT);
  ASSERT_TRUE(server.start());
  EXPECT_FALSE(server.start());
}

TEST(TcpServerIntegration, should_return_valid_socket_when_agent_connects) {
  const std::uint16_t port = TestConstants::TCP_SERVER_AGENT_CONNECT_PORT;
  TcpServer server(port);
  ASSERT_TRUE(server.start());

  std::promise<void> serverReady;
  std::shared_future<void> serverReadyFuture =
      serverReady.get_future().share();

  std::thread connector([&] {
    serverReadyFuture.wait();
    std::unique_ptr<ISocket> agentSocket = SocketFactory::createTCP();
    if (agentSocket) {
      agentSocket->connect("127.0.0.1", port);
    }
  });

  serverReady.set_value();
  auto accepted = server.acceptAgent();
  connector.join();

  EXPECT_NE(accepted, nullptr);
  EXPECT_TRUE(accepted->isValid());
}

TEST(TcpServerIntegration, should_receive_bytes_sent_by_connected_agent) {
  const std::uint16_t port = TestConstants::TCP_SERVER_ECHO_PORT;
  TcpServer server(port);
  ASSERT_TRUE(server.start());

  std::promise<void> serverReady;
  std::shared_future<void> serverReadyFuture =
      serverReady.get_future().share();
  std::promise<std::optional<ResponsePayload>> responsePromise;
  std::future<std::optional<ResponsePayload>> responseFuture =
      responsePromise.get_future();

  std::thread agentThread([&] {
    serverReadyFuture.wait();
    std::optional<ResponsePayload> response;

    auto socket = SocketFactory::createTCP();
    if (socket && socket->connect("127.0.0.1", port)) {
      AgentSession agentSession(std::move(socket));
      RegisterPayload registerPayload;
      registerPayload.os_type = OSType::LINUX;
      registerPayload.arch = ArchType::X64;
      registerPayload.hostname = "tcp-server-agent";
      const std::vector<std::uint8_t> registerBytes =
          ProtocolSerializer::serializeRegisterPayload(registerPayload);
      const auto registerFrame =
          buildRawFrame(MessageType::REGISTER, registerBytes);
      if (agentSession.send(registerFrame).ok()) {
        const SocketResult recvResult = agentSession.receiveIntoBuffer();
        if (recvResult.ok() && recvResult.bytesTransferred > 0) {
          std::optional<Frame> frame = agentSession.tryExtractFrame();
          if (frame.has_value() && frame->header.type == MessageType::RESPONSE) {
            response = ProtocolParser::parseResponsePayload(frame->payload);
          }
        }
      }
    }

    responsePromise.set_value(response);
  });

  serverReady.set_value();
  auto serverSocket = server.acceptAgent();
  ASSERT_NE(serverSocket, nullptr);
  AgentSession serverSession(std::move(serverSocket));

  serverSession.receiveIntoBuffer();
  std::optional<Frame> registerFrame = serverSession.tryExtractFrame();
  ASSERT_TRUE(registerFrame.has_value());
  EXPECT_EQ(registerFrame->header.type, MessageType::REGISTER);

    ResponsePayload responsePayload;
    responsePayload.id = 7;
    responsePayload.status = ResponseStatus::OK;
    responsePayload.total_chunks = 1;
    responsePayload.chunk_index = 0;
    responsePayload.data = "pong";
    const std::vector<std::uint8_t> responseBytes =
      ProtocolSerializer::serializeResponsePayload(responsePayload);
    ASSERT_TRUE(serverSession
            .send(buildRawFrame(MessageType::RESPONSE, responseBytes))
            .ok());

  agentThread.join();
  const std::optional<ResponsePayload> response = responseFuture.get();
  ASSERT_TRUE(response.has_value());
  EXPECT_EQ(response->id, 7);
  EXPECT_EQ(response->status, ResponseStatus::OK);
  EXPECT_EQ(response->data, "pong");
}

TEST(TcpServerIntegration, should_report_loopback_address_for_connected_agent) {
  const std::uint16_t port = TestConstants::TCP_SERVER_REMOTE_ADDR_PORT;
  TcpServer server(port);
  ASSERT_TRUE(server.start());

  std::promise<void> serverReady;
  std::shared_future<void> serverReadyFuture =
      serverReady.get_future().share();
  std::promise<void> serverDone;
  std::shared_future<void> serverDoneFuture =
      serverDone.get_future().share();

  std::thread connector([&] {
    serverReadyFuture.wait();
    auto socket = SocketFactory::createTCP();
    if (socket) {
      socket->connect("127.0.0.1", port);
      serverDoneFuture.wait();
    }
  });

  serverReady.set_value();
  auto accepted = server.acceptAgent();
  ASSERT_NE(accepted, nullptr);
  EXPECT_EQ(accepted->remoteAddress(), "127.0.0.1");
  EXPECT_GT(accepted->remotePort(), 0);

  serverDone.set_value();
  connector.join();
}
