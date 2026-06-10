#include <gtest/gtest.h>

#include <future>
#include <optional>
#include <thread>

#include "agent_session.hpp"
#include "helpers_test.hpp"
#include "poller/epoller.hpp"
#include "protocol/protocol_parser.hpp"
#include "reactor.hpp"
#include "server_dispatcher.hpp"
#include "socket/socket_factory.hpp"
#include "tcp_server.hpp"
#include "test_constants.hpp"

#if !defined(__linux__)

TEST(ReactorIntegration, NotSupportedOnThisPlatform) {
  GTEST_SKIP() << "Reactor epoll integration is Linux-only";
}

#else

#include <unistd.h>

namespace {

// Starts the server and reactor in a background thread.
// Sets reactorReady immediately before reactor.run() — at that point
// server.start() has returned so the OS is already listening on the port,
// meaning a client can connect even before the first epoll_wait fires.
// The OS queues incoming connections in the backlog; the reactor drains
// them on the first epoll_wait iteration.
std::thread startReactorThread(TcpServer& server, Reactor& reactor,
                               std::promise<void>& reactorReady) {
  server.start();
  server.setNonBlocking();
  return std::thread([&reactor, &reactorReady] {
    reactorReady.set_value();
    reactor.run();
  });
}

}  // namespace

// ① Happy path — agent connects and sends REGISTER.
TEST(ReactorIntegration, should_accept_register_without_error) {
  const std::uint16_t port = TestConstants::REACTOR_HAPPY_PATH_PORT;
  TcpServer server(port);
  Epoller epoller;
  ServerDispatcher dispatcher;
  Reactor reactor(server, dispatcher, epoller);

  std::promise<void> reactorReady;
  auto reactorThread = startReactorThread(server, reactor, reactorReady);
  reactorReady.get_future().wait();

  auto socket = SocketFactory::createTCP();
  ASSERT_TRUE(socket->connect("127.0.0.1", port));

  const auto registerFrame =
      makeRawFrame(MessageType::REGISTER, makeRegisterPayload());
  EXPECT_TRUE(socket->send(registerFrame).ok());

  reactor.stop();
  reactorThread.join();
}

// ② Protocol enforcement — agent sends COMMAND before REGISTER → ERROR back.
TEST(ReactorIntegration,
     should_send_error_when_first_message_is_not_register) {
  const std::uint16_t port = TestConstants::REACTOR_INVALID_FIRST_MESSAGE_PORT;
  TcpServer server(port);
  Epoller epoller;
  ServerDispatcher dispatcher;
  Reactor reactor(server, dispatcher, epoller);

  std::promise<void> reactorReady;
  auto reactorThread = startReactorThread(server, reactor, reactorReady);
  reactorReady.get_future().wait();

  auto socket = SocketFactory::createTCP();
  ASSERT_TRUE(socket->connect("127.0.0.1", port));

  const auto commandFrame = makeRawFrame(MessageType::COMMAND);
  ASSERT_TRUE(socket->send(commandFrame).ok());

  AgentSession session(std::move(socket));
  session.receiveIntoBuffer();
  std::optional<Frame> frame = session.tryExtractFrame();
  ASSERT_TRUE(frame.has_value());
  EXPECT_EQ(frame->header.type, MessageType::ERROR);

  reactor.stop();
  session.close();
  reactorThread.join();
}

// ③ Resilience — first agent disconnects, second agent connects and registers.
// The second connection is made without sleeping: the OS queues it in the
// backlog immediately after the first socket closes, and the reactor drains
// both events (disconnect + new connection) on the next epoll_wait.
TEST(ReactorIntegration,
     should_keep_serving_after_first_agent_disconnects) {
  const std::uint16_t port = TestConstants::REACTOR_DISCONNECT_PORT;
  TcpServer server(port);
  Epoller epoller;
  ServerDispatcher dispatcher;
  Reactor reactor(server, dispatcher, epoller);

  std::promise<void> reactorReady;
  auto reactorThread = startReactorThread(server, reactor, reactorReady);
  reactorReady.get_future().wait();

  // First agent — scope exit closes the socket, reactor sees EPOLLHUP
  {
    auto socket = SocketFactory::createTCP();
    ASSERT_TRUE(socket->connect("127.0.0.1", port));
    const auto registerFrame =
        makeRawFrame(MessageType::REGISTER, makeRegisterPayload());
    EXPECT_TRUE(socket->send(registerFrame).ok());
  }

  // Second agent — connects immediately; OS queues it in the backlog
  {
    auto socket = SocketFactory::createTCP();
    ASSERT_TRUE(socket->connect("127.0.0.1", port));
    const auto registerFrame =
        makeRawFrame(MessageType::REGISTER, makeRegisterPayload());
    EXPECT_TRUE(socket->send(registerFrame).ok());
  }

  reactor.stop();
  reactorThread.join();
}

#endif