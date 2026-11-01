#include "prevention_recovery.h"
#include <sstream>
#include <map>

namespace OSManager {

SimpleJson::JsonValue PolicyCheckItem::to_json() const {
    std::map<std::string, SimpleJson::JsonValue> obj;
    obj["process_id"] = SimpleJson::JsonValue(process_id);
    obj["requested_resource"] = SimpleJson::JsonValue(requested_resource);
    obj["highest_held_resource"] = SimpleJson::JsonValue(highest_held_resource);
    obj["complies"] = SimpleJson::JsonValue(complies);
    obj["reason"] = SimpleJson::JsonValue(reason);
    return SimpleJson::JsonValue(obj);
}

SimpleJson::JsonValue PreventionResult::to_json() const {
    std::map<std::string, SimpleJson::JsonValue> obj;
    obj["policy_enforced"] = SimpleJson::JsonValue(policy_enforced);
    obj["resource_order"] = SimpleJson::make_array(resource_order);

    std::vector<SimpleJson::JsonValue> chks;
    for (const auto& c : checks) chks.push_back(c.to_json());
    obj["checks"] = SimpleJson::JsonValue(chks);

    obj["all_compliant"] = SimpleJson::JsonValue(all_compliant);
    obj["circular_wait_explanation"] = SimpleJson::JsonValue(circular_wait_explanation);

    std::vector<SimpleJson::JsonValue> tos;
    for (const auto& t : trade_offs) tos.push_back(SimpleJson::JsonValue(t));
    obj["trade_offs"] = SimpleJson::JsonValue(tos);

    return SimpleJson::JsonValue(obj);
}

SimpleJson::JsonValue RecoveryResult::to_json() const {
    std::map<std::string, SimpleJson::JsonValue> obj;
    obj["terminated_process_id"] = SimpleJson::JsonValue(terminated_process_id);
    obj["resources_released"] = SimpleJson::make_array(resources_released);
    obj["previous_available"] = SimpleJson::make_array(previous_available);
    obj["updated_available"] = SimpleJson::make_array(updated_available);
    obj["remaining_allocation"] = SimpleJson::make_matrix(remaining_allocation);
    obj["remaining_request"] = SimpleJson::make_matrix(remaining_request);
    obj["remaining_processes"] = SimpleJson::make_array(remaining_processes);
    obj["recheck_result"] = recheck_result.to_json();
    obj["can_now_proceed"] = SimpleJson::JsonValue(can_now_proceed);
    obj["message"] = SimpleJson::JsonValue(message);

    return SimpleJson::JsonValue(obj);
}

PreventionResult simulate_prevention_policy(
    const std::vector<std::vector<int>>& allocation,
    const std::vector<std::vector<int>>& pending_requests,
    const std::vector<int>& resource_order
) {
    PreventionResult res;
    res.policy_enforced = true;
    res.resource_order = resource_order;
    res.all_compliant = true;

    // Create rank map: resource_id -> rank index (0, 1, 2, ...)
    std::map<int, int> rank_map;
    for (size_t rank = 0; rank < resource_order.size(); ++rank) {
        rank_map[resource_order[rank]] = static_cast<int>(rank);
    }

    int num_proc = static_cast<int>(allocation.size());
    int num_res = static_cast<int>(resource_order.size());

    for (int i = 0; i < num_proc; ++i) {
        // Find highest rank resource currently held by P_i
        int highest_held_res = -1;
        int highest_held_rank = -1;

        for (int j = 0; j < num_res; ++j) {
            if (j < static_cast<int>(allocation[i].size()) && allocation[i][j] > 0) {
                int r_rank = rank_map[j];
                if (r_rank > highest_held_rank) {
                    highest_held_rank = r_rank;
                    highest_held_res = j;
                }
            }
        }

        // Check each requested resource for P_i
        for (int j = 0; j < num_res; ++j) {
            if (i < static_cast<int>(pending_requests.size()) && j < static_cast<int>(pending_requests[i].size()) && pending_requests[i][j] > 0) {
                PolicyCheckItem item;
                item.process_id = i;
                item.requested_resource = j;
                item.highest_held_resource = highest_held_res;

                int req_rank = rank_map[j];

                if (highest_held_res == -1) {
                    // Holds no resources yet, request is trivially compliant
                    item.complies = true;
                    item.reason = "Process P" + std::to_string(i) + " holds no resources currently. Request for R" +
                                  std::to_string(j) + " (rank " + std::to_string(req_rank) + ") is ALLOWED.";
                } else if (req_rank > highest_held_rank) {
                    // Requesting strictly higher rank
                    item.complies = true;
                    item.reason = "Request for R" + std::to_string(j) + " (rank " + std::to_string(req_rank) +
                                  ") is higher than held R" + std::to_string(highest_held_res) +
                                  " (rank " + std::to_string(highest_held_rank) + "). ALLOWED by policy.";
                } else {
                    // Violation!
                    item.complies = false;
                    res.all_compliant = false;
                    item.reason = "VIOLATION: Process P" + std::to_string(i) + " holds R" +
                                  std::to_string(highest_held_res) + " (rank " + std::to_string(highest_held_rank) +
                                  "), but requested R" + std::to_string(j) + " (rank " + std::to_string(req_rank) +
                                  "). Requests for lower/equal rank without releasing higher resources are DENIED to prevent circular wait.";
                }
                res.checks.push_back(item);
            }
        }
    }

    std::ostringstream ss;
    ss << "Deadlock Prevention Rule: Strict Global Resource Ordering (Havender's Policy). "
       << "Each resource type is assigned a unique integer rank: ";
    for (size_t r = 0; r < resource_order.size(); ++r) {
        if (r > 0) ss << " < ";
        ss << "R" << resource_order[r];
    }
    ss << ". A process may only request resource R_j if rank(R_j) > rank(R_held) for all resources it currently holds. "
       << "Mathematical Proof: In any circular wait cycle (P0 -> P1 -> ... -> Pk -> P0), the ranks of requested resources "
       << "must strictly increase at each hop. Since a finite sequence of strictly increasing integers cannot loop back on itself "
       << "(r_0 < r_1 < ... < r_k < r_0 is impossible), CIRCULAR WAIT IS MATHEMATICALLY IMPOSSIBLE.";
    res.circular_wait_explanation = ss.str();

    res.trade_offs = {
        "Reduced System Concurrency: Processes must acquire resources in predetermined order even if they could safely use them out-of-order.",
        "Under-utilization of Resources: A process must hold resources earlier or release held resources prematurely to satisfy rank constraints.",
        "Programmer / Architectural Burden: The system designer must establish and maintain a global ranking across all shared subsystems.",
        "Potential Starvation: Processes requiring high-ranked resources alongside low-ranked resources may wait longer if earlier allocations are delayed."
    };

    return res;
}

RecoveryResult simulate_recovery_termination(
    int process_id,
    const std::vector<std::vector<int>>& allocation,
    const std::vector<std::vector<int>>& request,
    const std::vector<int>& available,
    bool is_single_instance
) {
    RecoveryResult res;
    res.terminated_process_id = process_id;
    res.previous_available = available;

    int num_proc = static_cast<int>(allocation.size());
    int num_res = static_cast<int>(available.size());

    if (process_id < 0 || process_id >= num_proc) {
        res.can_now_proceed = false;
        res.message = "Error: Invalid process identifier P" + std::to_string(process_id);
        return res;
    }

    // Resources released = allocation[process_id]
    res.resources_released = allocation[process_id];
    res.updated_available = available;
    for (int j = 0; j < num_res; ++j) {
        res.updated_available[j] += allocation[process_id][j];
    }

    // Build remaining system
    res.remaining_allocation = allocation;
    res.remaining_request = request;

    // Zero out the terminated process's allocation and request
    for (int j = 0; j < num_res; ++j) {
        res.remaining_allocation[process_id][j] = 0;
        res.remaining_request[process_id][j] = 0;
    }

    for (int i = 0; i < num_proc; ++i) {
        if (i != process_id) {
            res.remaining_processes.push_back(i);
        }
    }

    // Recheck deadlock detection
    res.recheck_result = run_deadlock_detection(res.remaining_allocation, res.remaining_request, res.updated_available, is_single_instance);

    res.can_now_proceed = !res.recheck_result.is_deadlocked;

    std::ostringstream msg;
    msg << "Process P" << process_id << " was aborted/terminated. Its held resources [";
    for (size_t j = 0; j < res.resources_released.size(); ++j) {
        if (j > 0) msg << ", ";
        msg << res.resources_released[j];
    }
    msg << "] were reclaimed back to Available [";
    for (size_t j = 0; j < res.updated_available.size(); ++j) {
        if (j > 0) msg << ", ";
        msg << res.updated_available[j];
    }
    msg << "]. ";

    if (res.can_now_proceed) {
        msg << "SUCCESS: Deadlock broken! All remaining processes can now proceed to execution.";
    } else {
        msg << "PARTIAL RECOVERY: Deadlock persists among remaining processes. Additional process termination may be required.";
    }

    res.message = msg.str();
    return res;
}

} // namespace OSManager
