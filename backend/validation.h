#ifndef VALIDATION_H
#define VALIDATION_H

#include <vector>
#include <string>
#include "json_helper.h"

namespace OSManager {

struct ValidationResult {
    bool valid = true;
    std::vector<std::string> errors;
    std::vector<std::string> warnings;
    std::vector<int> calculated_totals;

    SimpleJson::JsonValue to_json() const;
};

// Validate overall system configuration (Allocation, Maximum, Available)
ValidationResult validate_configuration(
    int num_processes,
    int num_resources,
    const std::vector<std::vector<int>>& allocation,
    const std::vector<std::vector<int>>& maximum,
    const std::vector<int>& available
);

// Validate a specific resource request
ValidationResult validate_request(
    int process_id,
    int num_processes,
    int num_resources,
    const std::vector<int>& request,
    const std::vector<int>& need_row,
    const std::vector<int>& available
);

} // namespace OSManager

#endif // VALIDATION_H
