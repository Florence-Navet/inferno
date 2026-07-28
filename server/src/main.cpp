
#include <cstdio>
#include <iostream>

#include "env_helper.hpp"
#include "poller/epoller.hpp"
#include "reactor.hpp"
#include "server_dispatcher.hpp"
#include "tcp_server.hpp"

int main() {
  std::setvbuf(stdout, nullptr, _IONBF, 0);
  std::setvbuf(stderr, nullptr, _IONBF, 0);
  Epoller poller;
  if (!poller.isValid()) {
    std::cerr << "[server] Failed to create epoll instance\n";
    return 1;
  }

  const uint16_t serverPort = EnvHelper::resolvePort();
  bool encryption = EnvHelper::resolveTlsEnabled();
  TcpServer server(serverPort, encryption);
  if (!server.start()) {
    std::cerr << "[server] Failed to bind/listen on port " << serverPort
              << '\n';
    return 1;
  }

  server.setNonBlocking();

  SessionManager manager;
  ServerDispatcher dispatcher(manager);
  Reactor reactor(server, dispatcher, poller, manager);
  reactor.run();
  return 0;
}