
#include <cstdio>
#include <iostream>

#include "dispatcher/server_dispatcher.hpp"
#include "env_helper.hpp"
#include "poller/epoller.hpp"
#include "reactor.hpp"
#include "repository/database_connection.hpp"
#include "repository_manager.hpp"
#include "service/agent_service.hpp"
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

  SessionManager sessionManager;
  DatabaseConnection db;

  AgentRepository agentRepository(db);
  AgentService agentService(agentRepository, sessionManager);

  CommandRepository commandRepository(db);
  CommandService commandService(commandRepository, sessionManager);

  // RepositoryManager repositoryManager;
  ServerDispatcher dispatcher(sessionManager, agentService, commandService);
  Reactor reactor(server, dispatcher, poller, sessionManager);
  reactor.run();
  return 0;
}