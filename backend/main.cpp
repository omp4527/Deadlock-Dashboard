#include "http_server.h"
#include "json_helper.h"
#include "validation.h"
#include "banker.h"
#include "deadlock_detection.h"
#include "prevention_recovery.h"
#include "test_runner.h"

#include <iostream>
#include <string>

using namespace OSManager;

int main(int argc, char* argv[]) {
    int port = 8080;
    const char* env_port = std::getenv("PORT");
    if (env_port != nullptr) {
        try {
            port = std::stoi(env_port);
        } catch (...) {
            port = 8080;
        }
    } else if (argc > 1) {
        try {
            port = std::stoi(argv[1]);
        } catch (...) {
            port = 8080;
        }
    }

    HttpServer server(port, "frontend");

    // 1. Health check endpoint
    server.get("/api/health", [](const HttpRequest&) {
        HttpResponse res;
        res.status_code = 200;
        res.body = "{\"status\": \"ok\", \"app\": \"OS Resource Manager Backend\", \"engine\": \"Native Kernel\", \"version\": \"1.0.0\"}";
        return res;
    });

    // 2. Validate configuration endpoint
    server.post("/api/validate", [](const HttpRequest& req) {
        HttpResponse res;
        try {
            SimpleJson::JsonValue root = SimpleJson::parse(req.body);
            int num_proc = root.get("num_processes").as_int();
            int num_res = root.get("num_resources").as_int();
            auto alloc = root.get("allocation").as_matrix_int();
            auto max = root.get("maximum").as_matrix_int();
            auto avail = root.get("available").as_vector_int();

            ValidationResult val = validate_configuration(num_proc, num_res, alloc, max, avail);
            res.status_code = 200;
            res.body = val.to_json().serialize();
        } catch (const std::exception& e) {
            res.status_code = 400;
            res.body = "{\"valid\": false, \"errors\": [\"Malformed request JSON: " + std::string(e.what()) + "\"]}";
        }
        return res;
    });

    // 3. Banker Calculate Need endpoint
    server.post("/api/banker/need", [](const HttpRequest& req) {
        HttpResponse res;
        try {
            SimpleJson::JsonValue root = SimpleJson::parse(req.body);
            auto alloc = root.get("allocation").as_matrix_int();
            auto max = root.get("maximum").as_matrix_int();

            auto need = calculate_need(alloc, max);
            std::map<std::string, SimpleJson::JsonValue> resp_obj;
            resp_obj["success"] = SimpleJson::JsonValue(true);
            resp_obj["need"] = SimpleJson::make_matrix(need);

            res.status_code = 200;
            res.body = SimpleJson::JsonValue(resp_obj).serialize();
        } catch (const std::exception& e) {
            res.status_code = 400;
            res.body = "{\"success\": false, \"error\": \"" + std::string(e.what()) + "\"}";
        }
        return res;
    });

    // 4. Banker Safety Algorithm endpoint
    server.post("/api/banker/safety", [](const HttpRequest& req) {
        HttpResponse res;
        try {
            SimpleJson::JsonValue root = SimpleJson::parse(req.body);
            int num_proc = root.get("num_processes").as_int();
            int num_res = root.get("num_resources").as_int();
            auto alloc = root.get("allocation").as_matrix_int();
            auto max = root.get("maximum").as_matrix_int();
            auto avail = root.get("available").as_vector_int();

            ValidationResult val = validate_configuration(num_proc, num_res, alloc, max, avail);
            if (!val.valid) {
                res.status_code = 400;
                res.body = "{\"success\": false, \"error\": \"Invalid input configuration\", \"validation\": " + val.to_json().serialize() + "}";
                return res;
            }

            auto need = calculate_need(alloc, max);
            SafetyResult safety = run_banker_safety(alloc, need, avail);

            std::map<std::string, SimpleJson::JsonValue> resp_obj;
            resp_obj["success"] = SimpleJson::JsonValue(true);
            resp_obj["safety"] = safety.to_json();
            resp_obj["need"] = SimpleJson::make_matrix(need);
            resp_obj["calculated_totals"] = SimpleJson::make_array(val.calculated_totals);

            res.status_code = 200;
            res.body = SimpleJson::JsonValue(resp_obj).serialize();
        } catch (const std::exception& e) {
            res.status_code = 400;
            res.body = "{\"success\": false, \"error\": \"" + std::string(e.what()) + "\"}";
        }
        return res;
    });

    // 5. Banker Resource Request Simulator endpoint
    server.post("/api/banker/request", [](const HttpRequest& req) {
        HttpResponse res;
        try {
            SimpleJson::JsonValue root = SimpleJson::parse(req.body);
            int process_id = root.get("process_id").as_int();
            auto request_vec = root.get("request").as_vector_int();
            auto alloc = root.get("allocation").as_matrix_int();
            auto max = root.get("maximum").as_matrix_int();
            auto avail = root.get("available").as_vector_int();

            RequestResult req_res = process_resource_request(process_id, request_vec, alloc, max, avail);

            res.status_code = 200;
            res.body = req_res.to_json().serialize();
        } catch (const std::exception& e) {
            res.status_code = 400;
            res.body = "{\"status\": \"INVALID\", \"message\": \"Malformed request: " + std::string(e.what()) + "\"}";
        }
        return res;
    });

    // 6. Deadlock Detection endpoint
    server.post("/api/deadlock/detect", [](const HttpRequest& req) {
        HttpResponse res;
        try {
            SimpleJson::JsonValue root = SimpleJson::parse(req.body);
            auto alloc = root.get("allocation").as_matrix_int();
            auto request_mat = root.get("request").as_matrix_int();
            auto avail = root.get("available").as_vector_int();
            bool is_single_instance = root.get("is_single_instance").as_bool(false);

            DeadlockDetectionResult det = run_deadlock_detection(alloc, request_mat, avail, is_single_instance);

            res.status_code = 200;
            res.body = det.to_json().serialize();
        } catch (const std::exception& e) {
            res.status_code = 400;
            res.body = "{\"error\": \"Malformed request: " + std::string(e.what()) + "\"}";
        }
        return res;
    });

    // 7. Deadlock Prevention Simulation endpoint
    server.post("/api/prevention/simulate", [](const HttpRequest& req) {
        HttpResponse res;
        try {
            SimpleJson::JsonValue root = SimpleJson::parse(req.body);
            auto alloc = root.get("allocation").as_matrix_int();
            auto pending_req = root.get("requests").as_matrix_int();
            auto order = root.get("resource_order").as_vector_int();

            PreventionResult prev = simulate_prevention_policy(alloc, pending_req, order);

            res.status_code = 200;
            res.body = prev.to_json().serialize();
        } catch (const std::exception& e) {
            res.status_code = 400;
            res.body = "{\"error\": \"Malformed request: " + std::string(e.what()) + "\"}";
        }
        return res;
    });

    // 8. Deadlock Recovery Simulation endpoint
    server.post("/api/recovery/terminate", [](const HttpRequest& req) {
        HttpResponse res;
        try {
            SimpleJson::JsonValue root = SimpleJson::parse(req.body);
            int process_id = root.get("process_id").as_int();
            auto alloc = root.get("allocation").as_matrix_int();
            auto request_mat = root.get("request").as_matrix_int();
            auto avail = root.get("available").as_vector_int();
            bool is_single_instance = root.get("is_single_instance").as_bool(false);

            RecoveryResult rec = simulate_recovery_termination(process_id, alloc, request_mat, avail, is_single_instance);

            res.status_code = 200;
            res.body = rec.to_json().serialize();
        } catch (const std::exception& e) {
            res.status_code = 400;
            res.body = "{\"error\": \"Malformed request: " + std::string(e.what()) + "\"}";
        }
        return res;
    });

    // 9. Run all test cases endpoint
    server.get("/api/run-tests", [](const HttpRequest&) {
        HttpResponse res;
        try {
            auto test_results = run_all_test_cases();
            std::vector<SimpleJson::JsonValue> arr;
            int passed_count = 0;
            for (const auto& t : test_results) {
                arr.push_back(t.to_json());
                if (t.passed) passed_count++;
            }

            std::map<std::string, SimpleJson::JsonValue> resp_obj;
            resp_obj["total_tests"] = SimpleJson::JsonValue(static_cast<int>(test_results.size()));
            resp_obj["passed_tests"] = SimpleJson::JsonValue(passed_count);
            resp_obj["all_passed"] = SimpleJson::JsonValue(passed_count == static_cast<int>(test_results.size()));
            resp_obj["tests"] = SimpleJson::JsonValue(arr);

            res.status_code = 200;
            res.body = SimpleJson::JsonValue(resp_obj).serialize();
        } catch (const std::exception& e) {
            res.status_code = 500;
            res.body = "{\"error\": \"Failed running tests: " + std::string(e.what()) + "\"}";
        }
        return res;
    });

    if (!server.start()) {
        std::cerr << "Failed to start server.\n";
        return 1;
    }

    return 0;
}
