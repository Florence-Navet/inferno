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

  std::ostringstream what;
  const int fd = incoming->getFd();
  if (!poller_.add(fd, WatchFlags::READ | WatchFlags::ERROR)) {
    Logger::error("reactor", "Failed to watch fd " + std::to_string(fd));
    return;
  }
  what << "Agent connected: " << incoming->remoteAddress() << ':'
       << incoming->remotePort();
  // agents_.emplace(fd, AgentConnection(std::move(incoming)));
  sessionManager_.addAgent(fd, std::move(incoming));
  Logger::info("reactor", what.str());
}

void Reactor::onAgentReady(int fileDescriptor) {
  // AgentConnection& session = agents_.at(fileDescriptor);
  AgentConnection& session = sessionManager_.getAgent(fileDescriptor);
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
  // agents_.erase(fileDescriptor);
  sessionManager_.removeAgent(fileDescriptor);
}