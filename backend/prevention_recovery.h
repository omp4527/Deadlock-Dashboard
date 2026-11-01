#ifndef PREVENTION_RECOVERY_H
#define PREVENTION_RECOVERY_H

#include <vector>
#include <string>
#include "json_helper.h"
#include "deadlock_detection.h"

namespace OSManager {

struct PolicyCheckItem {
    int process_id;
    int requested_resource;
    int highest_held_resource;
    bool complies;
    std::string reason;

    SimpleJson::JsonValue to_json() const;
};

struct PreventionResult {
    bool policy_enforced;
    std::vector<int> resource_order; // e.g. [0, 1, 2] meaning R0 < R1 < R2
    std::vector<PolicyCheckItem> checks;
    bool all_compliant;
    std::string circular_wait_explanation;
    std::vector<std::string> trade_offs;

    SimpleJson::JsonValue to_json() const;
};

struct RecoveryResult {
    int terminated_process_id;
    std::vector<int> resources_released;
    std::vector<int> previous_available;
    std::vector<int> updated_available;
    std::vector<std::vector<int>> remaining_allocation;
    std::vector<std::vector<int>> remaining_request;
    std::vector<int> remaining_processes;
    DeadlockDetectionResult recheck_result;
    bool can_now_proceed;
    std::string message;

    SimpleJson::JsonValue to_json() const;
};

// Simulate Deadlock Prevention via Hierarchical Resource Ordering (Havender's Policy)
PreventionResult simulate_prevention_policy(
    const std::vector<std::vector<int>>& allocation,
    const std::vector<std::vector<int>>& pending_requests,
    const std::vector<int>& resource_order
);

// Simulate Deadlock Recovery via Process Termination
RecoveryResult simulate_recovery_termination(
    int process_id,
    const std::vector<std::vector<int>>& allocation,
    const std::vector<std::vector<int>>& request,
    const std::vector<int>& available,
    bool is_single_instance = false
);

} // namespace OSManager

#endif // PREVENTION_RECOVERY_H
