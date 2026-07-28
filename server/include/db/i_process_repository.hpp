#ifndef I_PROCESS_REPOSITORY_HPP
#define I_PROCESS_REPOSITORY_HPP

#include <string>
#include <vector>

#include "protocol/lptf_protocol.hpp"

class IProcessRepository {
 public:
  virtual ~IProcessRepository() = default;

  // Creates a snapshot row then bulk-inserts the process list.
  // Called by ServerDispatcher when a RUNNING_PROCESSES response arrives.
  // TODO Replace by ResponseRepository?
  virtual void save(const std::string& agentId,
                    const std::vector<ProcessInfo>& processes) = 0;

  // Most recent snapshot for one agent — displayed in the dashboard table.
  virtual std::vector<ProcessInfo> findLatest(
      const std::string& agentId) = 0;
};

#endif