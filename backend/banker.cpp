#include "banker.h"
#include <sstream>

namespace OSManager {

SimpleJson::JsonValue SafetyStep::to_json() const {
    std::map<std::string, SimpleJson::JsonValue> obj;
    obj["step_number"] = SimpleJson::JsonValue(step_number);
    obj["process_id"] = SimpleJson::JsonValue(process_id);
    obj["work_before"] = SimpleJson::make_array(work_before);
    obj["need"] = SimpleJson::make_array(need);
    obj["allocation_added"] = SimpleJson::make_array(allocation_added);
    obj["work_after"] = SimpleJson::make_array(work_after);

    std::vector<SimpleJson::JsonValue> fin;
    for (bool b : finish_state) fin.push_back(SimpleJson::JsonValue(b));
    obj["finish_state"] = SimpleJson::JsonValue(fin);
    obj["explanation"] = SimpleJson::JsonValue(explanation);

    return SimpleJson::JsonValue(obj);
}

SimpleJson::JsonValue SafetyResult::to_json() const {
    std::map<std::string, SimpleJson::JsonValue> obj;
    obj["is_safe"] = SimpleJson::JsonValue(is_safe);
    obj["safe_sequence"] = SimpleJson::make_array(safe_sequence);

    std::vector<SimpleJson::JsonValue> trace_arr;
    for (const auto& s : trace) trace_arr.push_back(s.to_json());
    obj["trace"] = SimpleJson::JsonValue(trace_arr);

    obj["unfinished_processes"] = SimpleJson::make_array(unfinished_processes);
    obj["explanation"] = SimpleJson::JsonValue(explanation);

    return SimpleJson::JsonValue(obj);
}

SimpleJson::JsonValue RequestResult::to_json() const {
    std::map<std::string, SimpleJson::JsonValue> obj;
    obj["status"] = SimpleJson::JsonValue(status);
    obj["process_id"] = SimpleJson::JsonValue(process_id);
    obj["request_vector"] = SimpleJson::make_array(request_vector);
    obj["message"] = SimpleJson::JsonValue(message);
    obj["reason"] = SimpleJson::JsonValue(reason);
    obj["state_modified"] = SimpleJson::JsonValue(state_modified);
    obj["safety_check"] = safety_check.to_json();
    obj["new_allocation"] = SimpleJson::make_matrix(new_allocation);
    obj["new_need"] = SimpleJson::make_matrix(new_need);
    obj["new_available"] = SimpleJson::make_array(new_available);

    return SimpleJson::JsonValue(obj);
}

std::vector<std::vector<int>> calculate_need(
    const std::vector<std::vector<int>>& allocation,
    const std::vector<std::vector<int>>& maximum
) {
    size_t num_proc = allocation.size();
    std::vector<std::vector<int>> need;
    need.resize(num_proc);

    for (size_t i = 0; i < num_proc; ++i) {
        size_t num_res = allocation[i].size();
        need[i].resize(num_res, 0);
        for (size_t j = 0; j < num_res; ++j) {
            int max_val = (i < maximum.size() && j < maximum[i].size()) ? maximum[i][j] : 0;
            int alloc_val = allocation[i][j];
            need[i][j] = max_val - alloc_val;
        }
    }
    return need;
}

SafetyResult run_banker_safety(
    const std::vector<std::vector<int>>& allocation,
    const std::vector<std::vector<int>>& need,
    const std::vector<int>& available
) {
    SafetyResult result;
    int num_proc = static_cast<int>(allocation.size());
    if (num_proc == 0) {
        result.is_safe = true;
        result.explanation = "No processes configured in system.";
        return result;
    }
    int num_res = static_cast<int>(available.size());

    // 1. Initialize Work = Available, Finish[i] = false
    std::vector<int> work = available;
    std::vector<bool> finish(num_proc, false);
    result.trace.clear();
    result.safe_sequence.clear();
    result.unfinished_processes.clear();

    int step_counter = 1;

    // 2. Repeat until no eligible process can be found
    while (true) {
        int found_idx = -1;

        for (int i = 0; i < num_proc; ++i) {
            if (!finish[i]) {
                bool can_finish = true;
                for (int j = 0; j < num_res; ++j) {
                    if (need[i][j] > work[j]) {
                        can_finish = false;
                        break;
                    }
                }
                if (can_finish) {
                    found_idx = i;
                    break;
                }
            }
        }

        if (found_idx != -1) {
            // Process found
            SafetyStep step;
            step.step_number = step_counter++;
            step.process_id = found_idx;
            step.work_before = work;
            step.need = need[found_idx];
            step.allocation_added = allocation[found_idx];

            // Work = Work + Allocation[i]
            for (int j = 0; j < num_res; ++j) {
                work[j] += allocation[found_idx][j];
            }
            finish[found_idx] = true;

            step.work_after = work;
            step.finish_state = finish;

            std::ostringstream ss;
            ss << "Process P" << found_idx << " satisfies Need <= Work. "
               << "Allocated resources are reclaimed. Work becomes updated.";
            step.explanation = ss.str();

            result.trace.push_back(step);
            result.safe_sequence.push_back(found_idx);
        } else {
            // No process found in this iteration
            break;
        }
    }

    // 3. Check if all processes finished
    bool all_finished = true;
    for (int i = 0; i < num_proc; ++i) {
        if (!finish[i]) {
            all_finished = false;
            result.unfinished_processes.push_back(i);
        }
    }

    result.is_safe = all_finished;
    if (all_finished) {
        std::ostringstream ss;
        ss << "System is in a SAFE state! All processes can execute to completion without deadlock. Safe sequence found: <";
        for (size_t k = 0; k < result.safe_sequence.size(); ++k) {
            if (k > 0) ss << ", ";
            ss << "P" << result.safe_sequence[k];
        }
        ss << ">.";
        result.explanation = ss.str();
    } else {
        std::ostringstream ss;
        ss << "System is in an UNSAFE state! Deadlock avoidance cannot guarantee that processes can finish without circular waiting. Unfinished processes: ";
        for (size_t k = 0; k < result.unfinished_processes.size(); ++k) {
            if (k > 0) ss << ", ";
            ss << "P" << result.unfinished_processes[k];
        }
        ss << ".";
        result.explanation = ss.str();
    }

    return result;
}

RequestResult process_resource_request(
    int process_id,
    const std::vector<int>& request,
    std::vector<std::vector<int>>& allocation,
    const std::vector<std::vector<int>>& maximum,
    std::vector<int>& available
) {
    RequestResult res;
    res.process_id = process_id;
    res.request_vector = request;
    res.state_modified = false;

    int num_proc = static_cast<int>(allocation.size());
    int num_res = static_cast<int>(available.size());

    std::vector<std::vector<int>> need = calculate_need(allocation, maximum);

    // Initial copies in case of reject
    res.new_allocation = allocation;
    res.new_need = need;
    res.new_available = available;

    // Check process ID validity
    if (process_id < 0 || process_id >= num_proc) {
        res.status = "INVALID";
        res.message = "Invalid process identifier P" + std::to_string(process_id);
        res.reason = "Process index out of range [0, " + std::to_string(num_proc - 1) + "].";
        return res;
    }

    // Check vector length
    if (request.size() != static_cast<size_t>(num_res)) {
        res.status = "INVALID";
        res.message = "Request vector dimension mismatch.";
        res.reason = "Expected " + std::to_string(num_res) + " resource elements.";
        return res;
    }

    // Check non-negative request
    for (int j = 0; j < num_res; ++j) {
        if (request[j] < 0) {
            res.status = "INVALID";
            res.message = "Negative resource request rejected.";
            res.reason = "Resource counts must be non-negative integers. Got " + std::to_string(request[j]) + " for R" + std::to_string(j) + ".";
            return res;
        }
    }

    // Rule 1: Request <= Need[process_id]
    for (int j = 0; j < num_res; ++j) {
        if (request[j] > need[process_id][j]) {
            res.status = "INVALID";
            res.message = "Error: Process exceeded declared maximum demand!";
            res.reason = "Process P" + std::to_string(process_id) + " requested " +
                         std::to_string(request[j]) + " instances of resource R" + std::to_string(j) +
                         ", but remaining Need is only " + std::to_string(need[process_id][j]) + ".";
            return res;
        }
    }

    // Rule 2: Request <= Available
    for (int j = 0; j < num_res; ++j) {
        if (request[j] > available[j]) {
            res.status = "WAITING";
            res.message = "Request deferred: Insufficient resources currently available.";
            res.reason = "Process P" + std::to_string(process_id) + " requested " +
                         std::to_string(request[j]) + " of R" + std::to_string(j) +
                         ", but only " + std::to_string(available[j]) + " are currently available. " +
                         "Process P" + std::to_string(process_id) + " must wait. System state remains unmodified.";
            return res;
        }
    }

    // Rule 3: Speculatively allocate and test safety
    std::vector<std::vector<int>> temp_alloc = allocation;
    std::vector<std::vector<int>> temp_need = need;
    std::vector<int> temp_avail = available;

    for (int j = 0; j < num_res; ++j) {
        temp_avail[j] -= request[j];
        temp_alloc[process_id][j] += request[j];
        temp_need[process_id][j] -= request[j];
    }

    // Run Safety Algorithm on tentative state
    SafetyResult tentative_safety = run_banker_safety(temp_alloc, temp_need, temp_avail);
    res.safety_check = tentative_safety;

    if (tentative_safety.is_safe) {
        // Safe: commit allocation
        allocation = temp_alloc;
        available = temp_avail;
        need = temp_need;

        res.status = "GRANTED";
        res.state_modified = true;
        res.message = "Request safely granted!";
        res.reason = "Speculative allocation leaves the system in a safe state with sequence <";
        for (size_t k = 0; k < tentative_safety.safe_sequence.size(); ++k) {
            if (k > 0) res.reason += ", ";
            res.reason += "P" + std::to_string(tentative_safety.safe_sequence[k]);
        }
        res.reason += ">. Resource changes committed to system state.";

        res.new_allocation = allocation;
        res.new_need = need;
        res.new_available = available;
    } else {
        // Unsafe: reject and restore original state
        res.status = "DENIED";
        res.state_modified = false;
        res.message = "Request denied: Allocation would lead to an unsafe state.";
        res.reason = "Granting this request leaves insufficient resources to guarantee completion of all processes without deadlock risk. "
                     "Original system allocation and available resources have been fully preserved.";
        // System state is NOT modified
    }

    return res;
}

} // namespace OSManager
