#include "poller/epoller.hpp"

#include <gtest/gtest.h>

#include <cstdint>
#include <future>
#include <thread>
#include <vector>

#if !defined(__linux__)

TEST(Epoller, NotSupportedOnThisPlatform) {
  GTEST_SKIP() << "Epoller is Linux-only";
}

#else

#include "socket/i_socket.hpp"
#include "socket/socket_factory.hpp"
#include "test_constants.hpp"
#include "test_tcp_server.hpp"

namespace {

struct ConnectedSockets {
  std::unique_ptr<ISocket> serverSide;
  std::unique_ptr<ISocket> clientSide;
};

ConnectedSockets makeConnectedSockets(std::uint16_t port) {
  TestTcpServer server(port);
  if (!server.start()) {
    return {};
  }

  std::promise<void> serverReady;
  std::shared_future<void> serverReadyFuture =
      serverReady.get_future().share();
  std::unique_ptr<ISocket> client;

  std::thread connector([&] {
    serverReadyFuture.wait();
    client = SocketFactory::createTCP();
    if (client) {
      client->connect("127.0.0.1", port);
    }
  });

  serverReady.set_value();
  std::unique_ptr<ISocket> serverSide = server.acceptAgent();
  connector.join();

  return {std::move(serverSide), std::move(client)};
}

void closeSockets(ConnectedSockets& sockets) {
  if (sockets.serverSide) {
    sockets.serverSide->close();
  }
  if (sockets.clientSide) {
    sockets.clientSide->close();
  }
}

}  // namespace

TEST(EpollerUnit, should_be_valid_after_construction) {
  Epoller epoller;
  EXPECT_TRUE(epoller.isValid());
}

TEST(EpollerUnit, should_return_true_when_add_called_with_valid_fd) {
  Epoller epoller;

  ConnectedSockets sockets =
      makeConnectedSockets(TestConstants::EPOLLER_ADD_PORT);
  ASSERT_NE(sockets.serverSide, nullptr);
  ASSERT_TRUE(epoller.add(sockets.serverSide->getFd(), WatchFlags::READ));
  closeSockets(sockets);
}
TEST(EpollerUnit, should_return_false_when_add_called_with_invalid_fd) {
  Epoller epoller;
  // ASSERT_TRUE(epoller.isValid());
  EXPECT_FALSE(epoller.add(-1, WatchFlags::READ));
}

TEST(EpollerUnit, should_return_true_when_remove_called_on_watched_fd) {
  Epoller epoller;
  // ASSERT_TRUE(epoller.isValid());
  ConnectedSockets sockets =
      makeConnectedSockets(TestConstants::EPOLLER_REMOVE_PORT);
  ASSERT_NE(sockets.serverSide, nullptr);
  epoller.add(sockets.serverSide->getFd(), WatchFlags::READ);

  EXPECT_TRUE(epoller.remove(sockets.serverSide->getFd()));
  closeSockets(sockets);
}

TEST(EpollerUnit, should_return_false_when_remove_called_on_unwatched_fd) {
  Epoller epoller;
  // ASSERT_TRUE(epoller.isValid());
  EXPECT_FALSE(epoller.remove(12345));
}


TEST(EpollerUnit, should_return_zero_event_on_timeout_when_no_data) {
  Epoller epoller;

  ConnectedSockets sockets =
      makeConnectedSockets(TestConstants::EPOLLER_TIMEOUT_PORT);
  ASSERT_NE(sockets.serverSide, nullptr);
  epoller.add(sockets.serverSide->getFd(), WatchFlags::READ);
  std::vector<ReadyEvent> events;
  const int ready = epoller.wait(events, 0);
  EXPECT_EQ(ready, 0);
  EXPECT_TRUE(events.empty());

  epoller.remove(sockets.serverSide->getFd());
  closeSockets(sockets);
}

TEST(EpollerIntegration, should_report_readable_event_when_data_arrives) {
  Epoller epoller;

  ConnectedSockets sockets =
      makeConnectedSockets(TestConstants::EPOLLER_READABLE_PORT);
  ASSERT_NE(sockets.serverSide, nullptr);
  ASSERT_NE(sockets.clientSide, nullptr);
  epoller.add(sockets.serverSide->getFd(), WatchFlags::READ);

  const std::vector<std::uint8_t> payload = {0xAB};
  ASSERT_TRUE(sockets.clientSide->send(payload).ok());

  std::vector<ReadyEvent> events;
  const int ready = epoller.wait(events, 100);
  ASSERT_GT(ready, 0);
  ASSERT_FALSE(events.empty());

  bool found = false;
  for (const auto& event : events) {
    if (event.fileDescriptor == sockets.serverSide->getFd()) {
      found = true;
      EXPECT_TRUE(event.readable);
      EXPECT_FALSE(event.error);
      break;
    }
  }
  EXPECT_TRUE(found);

  epoller.remove(sockets.serverSide->getFd());
  closeSockets(sockets);
}

TEST(EpollerIntegration, should_continue_after_one_fd_removed) {
  Epoller epoller;
  ASSERT_TRUE(epoller.isValid());

  ConnectedSockets firstSockets =
      makeConnectedSockets(TestConstants::EPOLLER_CONTINUE_PORT_1);
  ConnectedSockets secondSockets =
      makeConnectedSockets(TestConstants::EPOLLER_CONTINUE_PORT_2);
  ASSERT_NE(firstSockets.serverSide, nullptr);
  ASSERT_NE(secondSockets.serverSide, nullptr);
  ASSERT_NE(firstSockets.clientSide, nullptr);
  ASSERT_NE(secondSockets.clientSide, nullptr);

  epoller.add(firstSockets.serverSide->getFd(), WatchFlags::READ);
  epoller.add(secondSockets.serverSide->getFd(), WatchFlags::READ);

  const std::vector<std::uint8_t> firstPayload = {0x11};
  ASSERT_TRUE(firstSockets.clientSide->send(firstPayload).ok());

  std::vector<ReadyEvent> events;
  const int readyFirst = epoller.wait(events, 100);
  ASSERT_GT(readyFirst, 0);

  bool firstFound = false;
  for (const auto& event : events) {
    if (event.fileDescriptor == firstSockets.serverSide->getFd()) {
      firstFound = true;
      EXPECT_TRUE(event.readable);
      break;
    }
  }
  EXPECT_TRUE(firstFound);

  epoller.remove(firstSockets.serverSide->getFd());
  closeSockets(firstSockets);

  const std::vector<std::uint8_t> secondPayload = {0x22};
  ASSERT_TRUE(secondSockets.clientSide->send(secondPayload).ok());

  events.clear();
  const int readySecond = epoller.wait(events, 100);
  ASSERT_GT(readySecond, 0);

  bool secondFound = false;
  for (const auto& event : events) {
    if (event.fileDescriptor == secondSockets.serverSide->getFd()) {
      secondFound = true;
      EXPECT_TRUE(event.readable);
      break;
    }
  }
  EXPECT_TRUE(secondFound);

  epoller.remove(secondSockets.serverSide->getFd());
  closeSockets(secondSockets);
}

#endif
