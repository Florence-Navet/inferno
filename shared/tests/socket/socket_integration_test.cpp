#include <gtest/gtest.h>
 
#include <future>
#include <thread>
#include <vector>
 
#include "socket/socket_factory.hpp"
#include "test_constants.hpp"
 
namespace {
 
// Echo server: binds, listens, signals ready, accepts one client,
// receives data, sends it back.
// The promise is set after listen() — at that point the OS is ready
// to queue incoming connections, so the client can safely connect.
void runEchoServer(std::uint16_t port, std::promise<void>& serverReady) {
  auto serverSocket = SocketFactory::createTCP();
  serverSocket->bind(port);
  serverSocket->listen();
  serverReady.set_value();     // unblocks the client — no busy-wait needed
 
  auto agentSocket = serverSocket->accept();
  std::vector<std::uint8_t> buffer(1024);
  auto receivedResult = agentSocket->recv(buffer.data(), buffer.size());
  agentSocket->send(buffer.data(), static_cast<std::size_t>(receivedResult.bytesTransferred));
}
 
}  // namespace
 
TEST(SocketIntegration, should_echo_sent_bytes_back_to_agent) {
  std::promise<void> serverReady;
  std::thread serverThread(runEchoServer, TestConstants::SOCKET_ECHO_PORT,
                           std::ref(serverReady));
  serverReady.get_future().wait();  // precise — no sleep, no busy-wait
 
  auto agentSocket = SocketFactory::createTCP();
  bool connected = agentSocket->connect("127.0.0.1", TestConstants::SOCKET_ECHO_PORT);
  ASSERT_TRUE(connected) << " Agent failed to connect to echo server on port " << TestConstants::SOCKET_ECHO_PORT;
 
  const std::vector<std::uint8_t> dataToSend = {0x01, 0x02, 0x03, 0x04};
  auto sendResult = agentSocket->send(dataToSend);
  EXPECT_TRUE(sendResult.ok());
  EXPECT_EQ(sendResult.bytesTransferred, 4);
 
  std::vector<std::uint8_t> received(1024);
  auto recvResult = agentSocket->recv(received.data(), received.size());
  EXPECT_TRUE(recvResult.ok());
  EXPECT_EQ(recvResult.bytesTransferred, 4);
 
  received.resize(static_cast<std::size_t>(recvResult.bytesTransferred));
  EXPECT_EQ(received, dataToSend);
 
  serverThread.join();
}
 
TEST(SocketIntegration,
     should_fail_to_connect_when_nothing_is_listening_on_port) {
  auto socket = SocketFactory::createTCP();
  bool connected = socket->connect("127.0.0.1", TestConstants::SOCKET_UNUSED_PORT);
  EXPECT_FALSE(connected);
}