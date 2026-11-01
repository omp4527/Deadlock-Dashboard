#include "test_runner.h"
#include "banker.h"
#include "deadlock_detection.h"
#include "prevention_recovery.h"
#include "validation.h"
#include <sstream>

namespace OSManager {

SimpleJson::JsonValue TestCaseResult::to_json() const {
    std::map<std::string, SimpleJson::JsonValue> obj;
    obj["test_id"] = SimpleJson::JsonValue(test_id);
    obj["test_name"] = SimpleJson::JsonValue(test_name);
    obj["system_condition"] = SimpleJson::JsonValue(system_condition);
    obj["input_summary"] = SimpleJson::JsonValue(input_summary);
    obj["expected_result"] = SimpleJson::JsonValue(expected_result);
    obj["actual_result"] = SimpleJson::JsonValue(actual_result);
    obj["passed"] = SimpleJson::JsonValue(passed);
    obj["analysis"] = SimpleJson::JsonValue(analysis);
    obj["detailed_data"] = detailed_data;
    return SimpleJson::JsonValue(obj);
}

std::vector<TestCaseResult> run_all_test_cases() {
    std::vector<TestCaseResult> results;

    // Baseline dataset (Silberschatz standard)
    std::vector<std::vector<int>> alloc_base = {
        {0, 1, 0},
        {2, 0, 0},
        {3, 0, 2},
        {2, 1, 1},
        {0, 0, 2}
    };
    std::vector<std::vector<int>> max_base = {
        {7, 5, 3},
        {3, 2, 2},
        {9, 0, 2},
        {2, 2, 2},
        {4, 3, 3}
    };
    std::vector<int> avail_base = {3, 3, 2};

    // ==========================================
    // TC1: Safe State
    // ==========================================
    {
        TestCaseResult tc;
        tc.test_id = "TC1";
        tc.test_name = "Safe State Verification";
        tc.system_condition = "5 processes (P0-P4), 3 resource types (A, B, C); Total Resources: [10, 5, 7].";
        tc.input_summary = "Alloc = [[0,1,0],[2,0,0],[3,0,2],[2,1,1],[0,0,2]], Max = [[7,5,3],[3,2,2],[9,0,2],[2,2,2],[4,3,3]], Avail = [3,3,2]";
        tc.expected_result = "System is in a SAFE state with complete safe sequence (e.g. <P1, P3, P4, P0, P2> or <P1, P3, P4, P2, P0>).";

        std::vector<std::vector<int>> need = calculate_need(alloc_base, max_base);
        SafetyResult safety = run_banker_safety(alloc_base, need, avail_base);

        std::ostringstream act;
        act << "State: " << (safety.is_safe ? "SAFE" : "UNSAFE") << ", Sequence: <";
        for (size_t k = 0; k < safety.safe_sequence.size(); ++k) {
            if (k > 0) act << ", ";
            act << "P" << safety.safe_sequence[k];
        }
        act << ">. Trace steps: " << safety.trace.size() << " processes completed.";
        tc.actual_result = act.str();

        tc.passed = safety.is_safe && (safety.safe_sequence.size() == 5);
        tc.analysis = "The Banker's safety algorithm correctly identified an initial safe state where Work initially equals [3, 3, 2] and satisfies Need requirements in sequence. No deadlock is possible.";
        tc.detailed_data = safety.to_json();
        results.push_back(tc);
    }

    // ==========================================
    // TC2: Request Exceeds Available
    // ==========================================
    {
        TestCaseResult tc;
        tc.test_id = "TC2";
        tc.test_name = "Request Exceeds Available Resources";
        tc.system_condition = "Safe baseline state with Available = [3, 3, 2]. P0 Need = [7, 4, 3].";
        tc.input_summary = "Process P0 requests [0, 4, 0]. (Request <= Need [7,4,3], but Request[1] = 4 > Available[1] = 3).";
        tc.expected_result = "Status: WAITING / Deferred. Request cannot be granted immediately. Allocation must remain unmodified.";

        std::vector<std::vector<int>> alloc_copy = alloc_base;
        std::vector<int> avail_copy = avail_base;
        std::vector<int> req = {0, 4, 0};

        RequestResult req_res = process_resource_request(0, req, alloc_copy, max_base, avail_copy);

        tc.actual_result = "Status: " + req_res.status + ", Decision: " + req_res.message +
                           ", Allocation modified: " + (req_res.state_modified ? "true" : "false");

        tc.passed = (req_res.status == "WAITING") && (!req_res.state_modified) && (alloc_copy == alloc_base) && (avail_copy == avail_base);
        tc.analysis = "Rule 2 of Banker's Algorithm verified: When Request <= Need but Request > Available, the process must wait. State rollback/non-modification was strictly preserved.";
        tc.detailed_data = req_res.to_json();
        results.push_back(tc);
    }

    // ==========================================
    // TC3: Request Produces an Unsafe State
    // ==========================================
    {
        TestCaseResult tc;
        tc.test_id = "TC3";
        tc.test_name = "Request Produces an Unsafe State";
        tc.system_condition = "Safe baseline state with Available = [3, 3, 2]. P0 Need = [7, 4, 3].";
        tc.input_summary = "Process P0 requests [0, 3, 0]. (Request <= Need and Request <= Available).";
        tc.expected_result = "Status: DENIED. Tentative allocation renders Work [3, 0, 2] unable to satisfy any process. Original state restored.";

        std::vector<std::vector<int>> alloc_copy = alloc_base;
        std::vector<int> avail_copy = avail_base;
        std::vector<int> req = {0, 3, 0};

        RequestResult req_res = process_resource_request(0, req, alloc_copy, max_base, avail_copy);

        tc.actual_result = "Status: " + req_res.status + ", Reason: " + req_res.message +
                           ", Allocation modified: " + (req_res.state_modified ? "true" : "false");

        tc.passed = (req_res.status == "DENIED") && (!req_res.state_modified) && (alloc_copy == alloc_base) && (avail_copy == avail_base);
        tc.analysis = "Rule 3 of Banker's Algorithm verified: Even though resources are physically available, granting the request would leave Available = [3, 0, 2], which fails all process Needs. The system detected the unsafe transition and rolled back to preserve system integrity.";
        tc.detailed_data = req_res.to_json();
        results.push_back(tc);
    }

    // ==========================================
    // TC4: Deadlock Detection & Recovery
    // ==========================================
    {
        TestCaseResult tc;
        tc.test_id = "TC4";
        tc.test_name = "Deadlock Detection & Recovery";
        tc.system_condition = "Multiple-instance resource system. Available = [0, 0, 0]. Allocation & Request matrices configured with circular dependency.";
        tc.input_summary = "Alloc = [[0,1,0],[2,0,0],[3,0,3],[2,1,1],[0,0,2]], Req = [[0,0,0],[2,0,2],[0,0,1],[1,0,0],[0,0,2]], Avail = [0,0,0].";
        tc.expected_result = "Deadlock detected in processes {P1, P2, P3, P4}. Terminating P2 releases [3, 0, 3] and allows all remaining processes to finish.";

        std::vector<std::vector<int>> alloc_d = {
            {0, 1, 0},
            {2, 0, 0},
            {3, 0, 3},
            {2, 1, 1},
            {0, 0, 2}
        };
        std::vector<std::vector<int>> req_d = {
            {0, 0, 0},
            {2, 0, 2},
            {0, 0, 1},
            {1, 0, 0},
            {0, 0, 2}
        };
        std::vector<int> avail_d = {0, 0, 0};

        DeadlockDetectionResult det = run_deadlock_detection(alloc_d, req_d, avail_d, false);
        RecoveryResult rec = simulate_recovery_termination(2, alloc_d, req_d, avail_d, false);

        std::ostringstream act;
        act << "Detection: " << (det.is_deadlocked ? "DEADLOCKED" : "NO DEADLOCK") << " (Deadlocked: ";
        for (size_t k = 0; k < det.deadlocked_processes.size(); ++k) {
            if (k > 0) act << ", ";
            act << "P" << det.deadlocked_processes[k];
        }
        act << "). Recovery: Terminated P2, can proceed = " << (rec.can_now_proceed ? "true" : "false");
        tc.actual_result = act.str();

        tc.passed = det.is_deadlocked && (det.deadlocked_processes.size() == 4) && rec.can_now_proceed;
        tc.analysis = "The multi-instance detection algorithm confirmed that only P0 can finish, leaving P1, P2, P3, P4 deadlocked. Aborting P2 released its 3 instances of A and 3 instances of C, enabling P1 and subsequent processes to complete.";
        
        std::map<std::string, SimpleJson::JsonValue> det_data;
        det_data["detection"] = det.to_json();
        det_data["recovery"] = rec.to_json();
        tc.detailed_data = SimpleJson::JsonValue(det_data);
        results.push_back(tc);
    }

    // ==========================================
    // TC5: Deadlock Prevention Policy
    // ==========================================
    {
        TestCaseResult tc;
        tc.test_id = "TC5";
        tc.test_name = "Deadlock Prevention (Strict Ordering)";
        tc.system_condition = "Fixed resource-acquisition ordering policy: R0 < R1 < R2 (ranks 0, 1, 2).";
        tc.input_summary = "P0 holds R1 (rank 1), requests R0 (rank 0) [Out of order]. P1 holds R0 (rank 0), requests R2 (rank 2) [In order].";
        tc.expected_result = "P0's request is flagged as a VIOLATION and blocked. P1's request is ALLOWED. Circular wait is eliminated.";

        std::vector<std::vector<int>> alloc_p = {
            {0, 1, 0}, // P0 holds R1
            {1, 0, 0}  // P1 holds R0
        };
        std::vector<std::vector<int>> pending_p = {
            {1, 0, 0}, // P0 requests R0
            {0, 0, 1}  // P1 requests R2
        };
        std::vector<int> order = {0, 1, 2};

        PreventionResult prev = simulate_prevention_policy(alloc_p, pending_p, order);

        bool p0_violated = false;
        bool p1_allowed = false;
        for (const auto& chk : prev.checks) {
            if (chk.process_id == 0 && chk.requested_resource == 0 && !chk.complies) p0_violated = true;
            if (chk.process_id == 1 && chk.requested_resource == 2 && chk.complies) p1_allowed = true;
        }

        tc.actual_result = "Policy Enforced: " + std::string(prev.policy_enforced ? "true" : "false") +
                           ", P0 Request Complies: " + (p0_violated ? "FALSE (Blocked)" : "TRUE") +
                           ", P1 Request Complies: " + (p1_allowed ? "TRUE (Allowed)" : "FALSE");

        tc.passed = prev.policy_enforced && p0_violated && p1_allowed;
        tc.analysis = "Havender's prevention algorithm strictly prevents Circular Wait by disallowing descending resource index acquisition without first releasing higher-ranked resources.";
        tc.detailed_data = prev.to_json();
        results.push_back(tc);
    }

    // ==========================================
    // TC6: Boundary & Invalid Inputs
    // ==========================================
    {
        TestCaseResult tc;
        tc.test_id = "TC6";
        tc.test_name = "Boundary & Invalid Inputs Handling";
        tc.system_condition = "Boundary conditions and erroneous inputs tested individually against validation and execution rules.";
        tc.input_summary = "Tests: 1) Avail = [0,0,0], 2) Process with Need = [0,0,0], 3) Invalid PID P99, 4) Negative req [-1,0,0], 5) Alloc > Max";
        tc.expected_result = "1 & 2 are treated as VALID boundary conditions. 3, 4 & 5 are detected as INVALID and rejected with specific error messages.";

        bool sub1_valid = true;  // zero available is valid boundary
        bool sub2_valid = true;  // zero need is valid boundary
        bool sub3_invalid = false; // invalid process ID
        bool sub4_invalid = false; // negative request
        bool sub5_invalid = false; // allocation > maximum

        // Subtest 1: Zero available
        std::vector<int> zero_avail = {0, 0, 0};
        ValidationResult v1 = validate_configuration(5, 3, alloc_base, max_base, zero_avail);
        sub1_valid = v1.valid;

        // Subtest 2: Process with zero remaining Need
        std::vector<std::vector<int>> max_with_zero_need = max_base;
        max_with_zero_need[1] = alloc_base[1]; // P1 Need becomes [0, 0, 0]
        std::vector<std::vector<int>> need_zero = calculate_need(alloc_base, max_with_zero_need);
        sub2_valid = (need_zero[1][0] == 0 && need_zero[1][1] == 0 && need_zero[1][2] == 0);

        // Subtest 3: Invalid Process ID P99
        std::vector<int> req3 = {0, 0, 0};
        std::vector<std::vector<int>> alloc_c3 = alloc_base;
        std::vector<int> avail_c3 = avail_base;
        RequestResult r3 = process_resource_request(99, req3, alloc_c3, max_base, avail_c3);
        sub3_invalid = (r3.status == "INVALID");

        // Subtest 4: Negative Request
        std::vector<int> req4 = {-1, 0, 0};
        RequestResult r4 = process_resource_request(0, req4, alloc_c3, max_base, avail_c3);
        sub4_invalid = (r4.status == "INVALID");

        // Subtest 5: Allocation > Maximum
        std::vector<std::vector<int>> bad_alloc = alloc_base;
        bad_alloc[0][0] = 8; // Max[0][0] is 7
        ValidationResult v5 = validate_configuration(5, 3, bad_alloc, max_base, avail_base);
        sub5_invalid = (!v5.valid);

        std::ostringstream act;
        act << "Zero Avail: " << (sub1_valid ? "Valid" : "Error")
            << ", Zero Need: " << (sub2_valid ? "Valid" : "Error")
            << ", Invalid PID: " << (sub3_invalid ? "Rejected" : "Allowed")
            << ", Neg Req: " << (sub4_invalid ? "Rejected" : "Allowed")
            << ", Alloc > Max: " << (sub5_invalid ? "Rejected" : "Allowed");
        tc.actual_result = act.str();

        tc.passed = sub1_valid && sub2_valid && sub3_invalid && sub4_invalid && sub5_invalid;
        tc.analysis = "The system clearly distinguished legal boundary states (zero available and zero remaining need are mathematically sound states) from illegal structural/input errors (out-of-bounds indices, negative values, and capacity violations).";

        std::map<std::string, SimpleJson::JsonValue> det_data;
        det_data["zero_available_valid"] = SimpleJson::JsonValue(sub1_valid);
        det_data["zero_need_valid"] = SimpleJson::JsonValue(sub2_valid);
        det_data["invalid_pid_rejected"] = SimpleJson::JsonValue(sub3_invalid);
        det_data["negative_req_rejected"] = SimpleJson::JsonValue(sub4_invalid);
        det_data["alloc_gt_max_rejected"] = SimpleJson::JsonValue(sub5_invalid);
        tc.detailed_data = SimpleJson::JsonValue(det_data);

        results.push_back(tc);
    }

    return results;
}

} // namespace OSManager
