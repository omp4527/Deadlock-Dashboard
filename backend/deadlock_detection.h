#ifndef DEADLOCK_DETECTION_H
#define DEADLOCK_DETECTION_H

#include <vector>
#include <string>
#include "json_helper.h"

namespace OSManager {

struct DetectionStep {
    int step_number;
    int process_id;
    std::vector<int> work_before;
    std::vector<int> request;
    std::vector<int> allocation_reclaimed;
    std::vector<int> work_after;
    std::vector<bool> finish_state;
    std::string reason;

    SimpleJson::JsonValue to_json() const;
};

struct GraphEdge {
    std::string from_node;
    std::string to_node;
    std::string type; // "ALLOCATION" (R -> P) or "REQUEST" (P -> R)

    SimpleJson::JsonValue to_json() const;
};

struct DeadlockDetectionResult {
    bool is_deadlocked;
    std::string resource_model; // "MULTIPLE_INSTANCE" or "SINGLE_INSTANCE"
    std::vector<int> deadlocked_processes;
    std::vector<int> finished_processes;
    std::vector<DetectionStep> steps;
    std::vector<GraphEdge> graph_edges;
    std::vector<std::string> detected_cycles;
    std::string explanation;

    SimpleJson::JsonValue to_json() const;
};

// Standard multi-instance detection algorithm (Silberschatz)
DeadlockDetectionResult run_deadlock_detection(
    const std::vector<std::vector<int>>& allocation,
    const std::vector<std::vector<int>>& request,
    const std::vector<int>& available,
    bool is_single_instance = false
);

// Helper to construct Resource Allocation Graph and detect cycles
void build_rag_and_detect_cycles(
    int num_proc,
    int num_res,
    const std::vector<std::vector<int>>& allocation,
    const std::vector<std::vector<int>>& request,
    std::vector<GraphEdge>& edges,
    std::vector<std::string>& cycles
);

} // namespace OSManager

#endif // DEADLOCK_DETECTION_H
