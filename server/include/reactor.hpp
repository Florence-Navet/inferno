#ifndef REACTOR_HPP
#define REACTOR_HPP

#include <sys/epoll.h>

#include <unordered_map>

// #include "agent_session.hpp"
#include "agent_connection.hpp"
#include "dashboard_connection.hpp"
#include "dispatcher/i_dispatcher.hpp"
#include "logger.hpp"
#include "poller/i_poller.hpp"
#include "tcp_server.hpp"

class Reactor {
 public:
  explicit Reactor(TcpServer& server, IDispatcher& dispatcher, IPoller& poller)
      : dispatcher_(dispatcher), server_(server), poller_(poller) {}
  Reactor() = delete;
  Reactor(const Reactor&) = delete;
  Reactor& operator=(const Reactor&) = delete;
  ~Reactor() = default;

  void run();
  void stop() { running_ = false; };

 private:
  IDispatcher& dispatcher_;
  TcpServer& server_;
  IPoller& poller_;
  //   Logger logger_{"reactor"};

  bool running_ = false;
  std::unordered_map<int, AgentConnection> agents_;
  std::shared_ptr<DashboardConnection> dashboard_;
  std::unordered_map<std::string, int>
      agentsByTarget_;  // agent id currently hostname, filedescriptor
  std::unordered_map<int, std::string>
      targetByFd_;  // agent id currently hostname, filedescriptor
  // internal helpers — each maps to one "something happened" situation
  void onNewConnection();
  void onAgentReady(int fileDescriptor);
  void onAgentDisconnected(int fileDescriptor);

  void createNewAgent(int fd, const std::unique_ptr<ISocket>& incoming,
                      const std::vector<std::uint8_t>& peek);
  void Reactor::createNewDashboard(int fd,
                                   const std::unique_ptr<ISocket>& incoming,
                                   const std::vector<std::uint8_t>& peek);
  void onDashboardReady();
  void onDashboardDisconnected();
};

#endif