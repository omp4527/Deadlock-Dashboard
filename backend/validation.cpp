#include "validation.h"

namespace OSManager {

SimpleJson::JsonValue ValidationResult::to_json() const {
    std::map<std::string, SimpleJson::JsonValue> obj;
    obj["valid"] = SimpleJson::JsonValue(valid);

    std::vector<SimpleJson::JsonValue> errs;
    for (const auto& e : errors) errs.push_back(SimpleJson::JsonValue(e));
    obj["errors"] = SimpleJson::JsonValue(errs);

    std::vector<SimpleJson::JsonValue> warns;
    for (const auto& w : warnings) warns.push_back(SimpleJson::JsonValue(w));
    obj["warnings"] = SimpleJson::JsonValue(warns);

    std::vector<SimpleJson::JsonValue> totals;
    for (int t : calculated_totals) totals.push_back(SimpleJson::JsonValue(t));
    obj["calculated_totals"] = SimpleJson::JsonValue(totals);

    return SimpleJson::JsonValue(obj);
}

ValidationResult validate_configuration(
    int num_processes,
    int num_resources,
    const std::vector<std::vector<int>>& allocation,
    const std::vector<std::vector<int>>& maximum,
    const std::vector<int>& available
) {
    ValidationResult res;

    // Dimension checks
    if (num_processes <= 0) {
        res.valid = false;
        res.errors.push_back("Process count must be strictly greater than 0 (got " + std::to_string(num_processes) + ").");
    }
    if (num_resources <= 0) {
        res.valid = false;
        res.errors.push_back("Resource type count must be strictly greater than 0 (got " + std::to_string(num_resources) + ").");
    }

    if (num_processes > 0 && allocation.size() != static_cast<size_t>(num_processes)) {
        res.valid = false;
        res.errors.push_back("Allocation matrix row count (" + std::to_string(allocation.size()) + ") does not match number of processes (" + std::to_string(num_processes) + ").");
    }
    if (num_processes > 0 && maximum.size() != static_cast<size_t>(num_processes)) {
        res.valid = false;
        res.errors.push_back("Maximum matrix row count (" + std::to_string(maximum.size()) + ") does not match number of processes (" + std::to_string(num_processes) + ").");
    }
    if (num_resources > 0 && available.size() != static_cast<size_t>(num_resources)) {
        res.valid = false;
        res.errors.push_back("Available vector size (" + std::to_string(available.size()) + ") does not match number of resource types (" + std::to_string(num_resources) + ").");
    }

    if (!res.valid) return res;

    // Available vector checks (non-negative)
    for (int j = 0; j < num_resources; ++j) {
        if (available[j] < 0) {
            res.valid = false;
            res.errors.push_back("Available resource R" + std::to_string(j) + " cannot be negative (" + std::to_string(available[j]) + ").");
        }
    }

    // Allocation and Maximum checks
    for (int i = 0; i < num_processes; ++i) {
        if (allocation[i].size() != static_cast<size_t>(num_resources)) {
            res.valid = false;
            res.errors.push_back("Allocation row for P" + std::to_string(i) + " has " + std::to_string(allocation[i].size()) + " columns, expected " + std::to_string(num_resources) + ".");
            continue;
        }
        if (maximum[i].size() != static_cast<size_t>(num_resources)) {
            res.valid = false;
            res.errors.push_back("Maximum row for P" + std::to_string(i) + " has " + std::to_string(maximum[i].size()) + " columns, expected " + std::to_string(num_resources) + ".");
            continue;
        }

        for (int j = 0; j < num_resources; ++j) {
            int alloc_val = allocation[i][j];
            int max_val = maximum[i][j];

            if (alloc_val < 0) {
                res.valid = false;
                res.errors.push_back("Allocation[" + std::to_string(i) + "][" + std::to_string(j) + "] is negative (" + std::to_string(alloc_val) + ").");
            }
            if (max_val < 0) {
                res.valid = false;
                res.errors.push_back("Maximum[" + std::to_string(i) + "][" + std::to_string(j) + "] is negative (" + std::to_string(max_val) + ").");
            }
            if (alloc_val > max_val) {
                res.valid = false;
                res.errors.push_back("Allocation exceeds Maximum claim for P" + std::to_string(i) + ", Resource R" + std::to_string(j) + " (" + std::to_string(alloc_val) + " > " + std::to_string(max_val) + ").");
            }
        }
    }

    // Calculate Total Resources Invariant: Total[j] = Sum of Allocations[i][j] + Available[j]
    res.calculated_totals.resize(num_resources, 0);
    for (int j = 0; j < num_resources; ++j) {
        int alloc_sum = 0;
        for (int i = 0; i < num_processes; ++i) {
            if (i < static_cast<int>(allocation.size()) && j < static_cast<int>(allocation[i].size())) {
                alloc_sum += allocation[i][j];
            }
        }
        int avail = (j < static_cast<int>(available.size())) ? available[j] : 0;
        res.calculated_totals[j] = alloc_sum + avail;
    }

    return res;
}

ValidationResult validate_request(
    int process_id,
    int num_processes,
    int num_resources,
    const std::vector<int>& request,
    const std::vector<int>& need_row,
    const std::vector<int>& available
) {
    ValidationResult res;

    // 1. Process identifier exists
    if (process_id < 0 || process_id >= num_processes) {
        res.valid = false;
        res.errors.push_back("Invalid process identifier P" + std::to_string(process_id) + ". Must be between P0 and P" + std::to_string(num_processes - 1) + ".");
        return res;
    }

    if (request.size() != static_cast<size_t>(num_resources)) {
        res.valid = false;
        res.errors.push_back("Request vector size (" + std::to_string(request.size()) + ") does not match resource types (" + std::to_string(num_resources) + ").");
        return res;
    }

    // 2. Request values are non-negative
    for (int j = 0; j < num_resources; ++j) {
        if (request[j] < 0) {
            res.valid = false;
            res.errors.push_back("Negative resource request for R" + std::to_string(j) + " (" + std::to_string(request[j]) + "). Values must be nonnegative integers.");
        }
    }
    if (!res.valid) return res;

    // 3. Request does not exceed remaining Need
    for (int j = 0; j < num_resources; ++j) {
        if (request[j] > need_row[j]) {
            res.valid = false;
            res.errors.push_back("Error: Process P" + std::to_string(process_id) + " exceeded its maximum claim! Request[" + std::to_string(j) + "] (" + std::to_string(request[j]) + ") > Need[" + std::to_string(j) + "] (" + std::to_string(need_row[j]) + ").");
        }
    }
    if (!res.valid) return res;

    // 4. Request exceeds Available (deferred/waiting condition, not an input syntax error)
    bool exceeds_available = false;
    for (int j = 0; j < num_resources; ++j) {
        if (request[j] > available[j]) {
            exceeds_available = true;
            res.warnings.push_back("Request exceeds currently Available resources for R" + std::to_string(j) + " (Requested: " + std::to_string(request[j]) + ", Available: " + std::to_string(available[j]) + "). Process must wait.");
        }
    }

    return res;
}

} // namespace OSManager
