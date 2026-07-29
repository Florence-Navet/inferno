#include <gtest/gtest.h>

#include <future>
#include <optional>
#include <thread>

// #include "agent_session.hpp"
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
#include "stubs/fake_agent_repository.hpp"
#include "stubs/fake_command_repository.hpp"
#include "stubs/fake_database_connection.hpp"
#include "stubs/spy_socket.hpp"
#include "tcp_server.hpp"

#if !defined(__linux__)

TEST(ReactorIntegration, NotSupportedOnThisPlatform) {
  GTEST_SKIP() << "Reactor epoll integration is Linux-only";
}

#else

#include <unistd.h>

class ReactorIntegrationTest : public ::testing::Test {
 public:
  // ReactorIntegrationTest()
  //     : poller((spy.makeUnique())), dispatcher(manager) {}
  // reactor(server, dispatcher, epoller, manager) {}

 protected:
  Epoller epoller;
  SessionManager manager;
  FakeDatabaseConnection fakeDb;
  FakeAgentRepository fakeAgents{fakeDb};
  FakeCommandRepository fakeCommands{fakeDb};

  // RepositoryManager repositoryManager;

  // ServerDispatcher dispatcher;
  // Lazy-initialized via SetUp()
  std::optional<RepositoryManager> repositoryManager;
  std::optional<ServerDispatcher> dispatcher;

  void SetUp() override {
    repositoryManager.emplace(
        std::make_unique<FakeDatabaseConnection>(fakeDb),
        std::make_unique<FakeAgentRepository>(fakeAgents),
        std::make_unique<FakeCommandRepository>(fakeCommands));

    // dispatcher = ServerDispatcher(manager, repositoryManager);
    dispatcher.emplace(manager, *repositoryManager);
  }

  // Reactor reactor;

  std::thread startReactorThread(TcpServer& server, Reactor& reactor,
                                 std::promise<void>& reactorReady) {
    server.start();
    server.setNonBlocking();
    return std::thread([&reactor, &reactorReady] {
      reactorReady.set_value();
      reactor.run();
    });
  }

  // std::thread startDashboardThread(ISocket& dashboard, std::uint16_t port,
  //                                  std::promise<void>& dashboardReady) {
  //   dashboard.connect(Common::SERVER_HOST, port);

  //   OsInfoPayload info = FrameBuilder::makeOsInfoPayload();
  //   std::vector<std::uint8_t> payload =
  //       ProtocolSerializer::serializeOsInfoPayload(info);

  //   Frame frame;
  //   frame.header =
  //       ProtocolHelper::createHeader(MessageType::DASHBOARD_REGISTER,
  //       payload);
  //   frame.payload = payload;
  //   std::vector<std::uint8_t> finalPayload =
  //       ProtocolSerializer::serializeFrame(frame);

  //   EXPECT_TRUE(socket->send(registerFrame).ok());
  //   dashboardReady.set_value();
  // }
};

// ① Happy path — agent connects and sends REGISTER.
TEST_F(ReactorIntegrationTest, should_accept_register_without_error) {
  const std::uint16_t port = Ports::Reactor::HAPPY_PATH_PORT;
  TcpServer server(port);
  Reactor reactor(server, *dispatcher, epoller, manager);

  std::promise<void> reactorReady;
  auto reactorThread = startReactorThread(server, reactor, reactorReady);
  reactorReady.get_future().wait();

  auto socket = SocketFactory::createTCP();
  ASSERT_TRUE(socket->connect(Common::SERVER_HOST, port));

  const auto registerFrame = FrameBuilder::makeRawFrame(
      MessageType::REGISTER, FrameBuilder::makeRawOsInfoPayload());
  EXPECT_TRUE(socket->send(registerFrame).ok());

  reactor.stop();
  reactorThread.join();
}

// ② Protocol enforcement — agent sends COMMAND before REGISTER → ERROR back.
TEST_F(ReactorIntegrationTest,
       should_send_error_when_first_message_is_not_register) {
  const std::uint16_t port = Ports::Reactor::INVALID_FIRST_MESSAGE_PORT;
  TcpServer server(port);
  Reactor reactor(server, *dispatcher, epoller, manager);

  std::promise<void> reactorReady;
  auto reactorThread = startReactorThread(server, reactor, reactorReady);
  reactorReady.get_future().wait();

  auto socket = SocketFactory::createTCP();
  ASSERT_TRUE(socket->connect(Common::SERVER_HOST, port));

  const auto commandFrame = FrameBuilder::makeRawFrame(MessageType::COMMAND);
  ASSERT_TRUE(socket->send(commandFrame).ok());

  AgentConnection session(std::move(socket));
  session.receiveIntoBuffer();
  std::optional<Frame> frame = session.tryExtractFrame();
  ASSERT_TRUE(frame.has_value());
  EXPECT_EQ(frame->header.type, MessageType::INFERNO_ERROR);

  reactor.stop();
  session.close();
  reactorThread.join();
}

// ③ Resilience — first agent disconnects, second agent connects and registers.
// The second connection is made without sleeping: the OS queues it in the
// backlog immediately after the first socket closes, and the reactor drains
// both events (disconnect + new connection) on the next epoll_wait.
TEST_F(ReactorIntegrationTest,
       should_keep_serving_after_first_agent_disconnects) {
  const std::uint16_t port = Ports::Reactor::DISCONNECT_PORT;
  TcpServer server(port);
  Reactor reactor(server, *dispatcher, epoller, manager);

  std::promise<void> reactorReady;
  auto reactorThread = startReactorThread(server, reactor, reactorReady);
  reactorReady.get_future().wait();

  // First agent — scope exit closes the socket, reactor sees EPOLLHUP
  {
    auto socket = SocketFactory::createTCP();
    ASSERT_TRUE(socket->connect(Common::SERVER_HOST, port));
    const auto registerFrame = FrameBuilder::makeRawFrame(
        MessageType::REGISTER, FrameBuilder::makeRawOsInfoPayload());
    EXPECT_TRUE(socket->send(registerFrame).ok());
  }

  // Second agent — connects immediately; OS queues it in the backlog
  {
    auto socket = SocketFactory::createTCP();
    ASSERT_TRUE(socket->connect(Common::SERVER_HOST, port));
    const auto registerFrame = FrameBuilder::makeRawFrame(
        MessageType::REGISTER, FrameBuilder::makeRawOsInfoPayload());
    EXPECT_TRUE(socket->send(registerFrame).ok());
  }

  reactor.stop();
  reactorThread.join();
}

#endif