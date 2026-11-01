#ifndef TEST_RUNNER_H
#define TEST_RUNNER_H

#include <vector>
#include <string>
#include "json_helper.h"

namespace OSManager {

struct TestCaseResult {
    std::string test_id;
    std::string test_name;
    std::string system_condition;
    std::string input_summary;
    std::string expected_result;
    std::string actual_result;
    bool passed;
    std::string analysis;
    SimpleJson::JsonValue detailed_data;

    SimpleJson::JsonValue to_json() const;
};

// Run all test cases TC1 through TC6 and return list of results
std::vector<TestCaseResult> run_all_test_cases();

} // namespace OSManager

#endif // TEST_RUNNER_H
