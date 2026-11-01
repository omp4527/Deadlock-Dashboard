#ifndef BANKER_H
#define BANKER_H

#include <vector>
#include <string>
#include "json_helper.h"
#include "validation.h"

namespace OSManager {

struct SafetyStep {
    int step_number;
    int process_id;
    std::vector<int> work_before;
    std::vector<int> need;
    std::vector<int> allocation_added;
    std::vector<int> work_after;
    std::vector<bool> finish_state;
    std::string explanation;

    SimpleJson::JsonValue to_json() const;
};

struct SafetyResult {
    bool is_safe;
    std::vector<int> safe_sequence;
    std::vector<SafetyStep> trace;
    std::vector<int> unfinished_processes;
    std::string explanation;

    SimpleJson::JsonValue to_json() const;
};

struct RequestResult {
    std::string status;       // "GRANTED", "DENIED", "WAITING", "INVALID"
    int process_id;
    std::vector<int> request_vector;
    std::string message;
    std::string reason;
    bool state_modified;
    SafetyResult safety_check;
    std::vector<std::vector<int>> new_allocation;
    std::vector<std::vector<int>> new_need;
    std::vector<int> new_available;

    SimpleJson::JsonValue to_json() const;
};

// Calculate Need[i][j] = Maximum[i][j] - Allocation[i][j]
std::vector<std::vector<int>> calculate_need(
    const std::vector<std::vector<int>>& allocation,
    const std::vector<std::vector<int>>& maximum
);

// Execute Banker's Safety Algorithm
SafetyResult run_banker_safety(
    const std::vector<std::vector<int>>& allocation,
    const std::vector<std::vector<int>>& need,
    const std::vector<int>& available
);

// Process a Resource Request
RequestResult process_resource_request(
    int process_id,
    const std::vector<int>& request,
    std::vector<std::vector<int>>& allocation,
    const std::vector<std::vector<int>>& maximum,
    std::vector<int>& available
);

} // namespace OSManager

#endif // BANKER_H
