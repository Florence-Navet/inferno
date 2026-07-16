#include "reactor.hpp"

#include <sstream>

void Reactor::run() {
  poller_.add(server_.getFd(), WatchFlags::READ | WatchFlags::ERROR);
  running_ = true;

  std::vector<ReadyEvent> events;

  while (running_) {
    const int firedCount = poller_.wait(events, -1);
    if (firedCount <= 0) {
      running_ = false;
    } else {
      for (const ReadyEvent& event : events) {
        if (event.fileDescriptor == server_.getFd()) {
          onNewConnection();
        } else if (event.readable) {
          onAgentReady(event.fileDescriptor);
        } else if (event.error) {
          onAgentDisconnected(event.fileDescriptor);
        }
      }
    }
  }
}

void Reactor::onNewConnection() {
  std::unique_ptr<ISocket> incoming = server_.acceptAgent();

  if (!incoming) return;

  const int fd = incoming->getFd();
  if (!poller_.add(fd, WatchFlags::READ | WatchFlags::ERROR)) {
    Logger::error("reactor", "Failed to watch fd " + std::to_string(fd));
    return;
  }

  std::vector<std::uint8_t> peek(4);
  const SocketResult result = incoming->recv(peek.data(), 4);

  if (!result.ok() || result.bytesTransferred != 4) {
    Logger::error("reactor", "Failed to peek identifier");
    poller_.remove(fd);
    return;
  }

  std::ostringstream what;
  std::string identifier(peek.begin(), peek.end());
  if (identifier == DASHBORD_IDENTIFIER) {
    what << "Dashboard connected: ";
    createNewDashboard(fd, incoming, peek);
  } else {
    what << "Agent connected: ";
    createNewAgent(fd, incoming, peek);
  }
  //  Logger::info("reactor", "Connection accepted on fd " +
  what << incoming->remoteAddress() << ':' << incoming->remotePort();
  //  std::to_string(fd));
  Logger::info("reactor", what.str());

  // if (incoming) {
  //   const int fd = incoming->getFd();
  //   if (!poller_.add(fd, WatchFlags::READ | WatchFlags::ERROR)) {
  //     std::ostringstream what;
  //     what << "Failed to watch agent fd " << fd;
  //     Logger::error("reactor", what.str());
  //     return;
  //   }
  //   std::ostringstream what;
  //   what << "Agent connected: " << incoming->remoteAddress() << ':'
  //        << incoming->remotePort();
  //   Logger::info("reactor", what.str());
  //   agents_.emplace(fd, AgentConnection(std::move(incoming)));
  // agents_.try_emplace(fd, std::move(incoming)); // no longer need moveable
  // constructor for logger
  // }
}

void Reactor::createNewAgent(int fd, const std::unique_ptr<ISocket>& incoming,
                             const std::vector<std::uint8_t>& peek) {
  std::unique_ptr<AgentConnection> agent =
      std::make_unique<AgentConnection>(std::move(incoming));
  agent->appendToBuffer(peek);  // Put those 4 bytes back in the buffer
  agents_.emplace(fd, std::move(*agent));
}

void Reactor::createNewDashboard(int fd,
                                 const std::unique_ptr<ISocket>& incoming,
                                 const std::vector<std::uint8_t>& peek) {
  std::shared_ptr<DashboardConnection> dashboard =
      std::make_shared<DashboardConnection>(std::move(incoming));
  dashboard->appendToBuffer(peek);
  dashboard_ = dashboard;
  dispatcher_.setDashboard(dashboard);
}

void Reactor::onAgentReady(int fileDescriptor) {
  AgentConnection& session = agents_.at(fileDescriptor);
  const SocketResult result = session.receiveIntoBuffer();

  if (!result.ok() || result.error == SocketStatus::CONNECTION_RESET ||
      result.bytesTransferred <= 0) {
    std::ostringstream what;
    what << "Agent " << fileDescriptor << " disconnected\n";
    Logger::info("reactor", what.str());
    // std::cout << "[server] Agent " << fileDescriptor << " disconnected\n";
    onAgentDisconnected(fileDescriptor);
    return;
  }

  while (std::optional<Frame> frame = session.tryExtractFrame()) {
    if (!session.getIsRegistered() &&
        frame->header.type != MessageType::REGISTER) {
      dispatcher_.sendError(session, ErrorType::INVALID_FORMAT,
                            "First message must be REGISTER");
    } else {
      dispatcher_.handleFrame(session, frame.value());
    }
  }
}

void Reactor::onAgentDisconnected(int fileDescriptor) {
  poller_.remove(fileDescriptor);
  agents_.erase(fileDescriptor);
}