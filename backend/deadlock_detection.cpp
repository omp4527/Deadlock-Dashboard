
#include "deadlock_detection.h"
#include <algorithm>
#include <set>
#include <sstream>

namespace OSManager {

// Convert Detection Step into JSON
SimpleJson::JsonValue DetectionStep::to_json() const {
  std::map<std::string, SimpleJson::JsonValue> obj;

  obj["step_number"] = SimpleJson::JsonValue(step_number);
  obj["process_id"] = SimpleJson::JsonValue(process_id);
  obj["work_before"] = SimpleJson::make_array(work_before);
  obj["request"] = SimpleJson::make_array(request);
  obj["allocation_reclaimed"] = SimpleJson::make_array(allocation_reclaimed);
  obj["work_after"] = SimpleJson::make_array(work_after);

  std::vector<SimpleJson::JsonValue> fin;

  for (bool b : finish_state) {
    fin.push_back(SimpleJson::JsonValue(b));
  }

  obj["finish_state"] = SimpleJson::JsonValue(fin);
  obj["reason"] = SimpleJson::JsonValue(reason);

  return SimpleJson::JsonValue(obj);
}

// Convert Graph Edge into JSON
SimpleJson::JsonValue GraphEdge::to_json() const {
  std::map<std::string, SimpleJson::JsonValue> obj;

  obj["from_node"] = SimpleJson::JsonValue(from_node);
  obj["to_node"] = SimpleJson::JsonValue(to_node);
  obj["type"] = SimpleJson::JsonValue(type);

  return SimpleJson::JsonValue(obj);
}

// Convert Deadlock Detection Result into JSON
SimpleJson::JsonValue DeadlockDetectionResult::to_json() const {
  std::map<std::string, SimpleJson::JsonValue> obj;

  obj["is_deadlocked"] = SimpleJson::JsonValue(is_deadlocked);
  obj["resource_model"] = SimpleJson::JsonValue(resource_model);
  obj["deadlocked_processes"] = SimpleJson::make_array(deadlocked_processes);
  obj["finished_processes"] = SimpleJson::make_array(finished_processes);

  std::vector<SimpleJson::JsonValue> s_arr;

  for (const auto &s : steps) {
    s_arr.push_back(s.to_json());
  }

  obj["steps"] = SimpleJson::JsonValue(s_arr);

  std::vector<SimpleJson::JsonValue> e_arr;

  for (const auto &e : graph_edges) {
    e_arr.push_back(e.to_json());
  }

  obj["graph_edges"] = SimpleJson::JsonValue(e_arr);

  std::vector<SimpleJson::JsonValue> c_arr;

  for (const auto &c : detected_cycles) {
    c_arr.push_back(SimpleJson::JsonValue(c));
  }

  obj["detected_cycles"] = SimpleJson::JsonValue(c_arr);
  obj["explanation"] = SimpleJson::JsonValue(explanation);

  return SimpleJson::JsonValue(obj);
}

// DFS Cycle Detection Function
static bool
dfs_rag_cycle(const std::string &current,
              const std::map<std::string, std::vector<std::string>> &adj,
              std::map<std::string, int> &color, std::vector<std::string> &path,
              std::vector<std::string> &cycles) {
  color[current] = 1;
  path.push_back(current);

  auto it = adj.find(current);

  if (it != adj.end()) {
    for (const auto &neighbor : it->second) {
      if (color[neighbor] == 1) {
        std::ostringstream cycle_str;

        size_t start_idx = 0;

        for (size_t i = 0; i < path.size(); i++) {
          if (path[i] == neighbor) {
            start_idx = i;
            break;
          }
        }

        for (size_t i = start_idx; i < path.size(); i++) {
          cycle_str << path[i] << " -> ";
        }

        cycle_str << neighbor;

        cycles.push_back(cycle_str.str());
      } else if (color[neighbor] == 0) {
        dfs_rag_cycle(neighbor, adj, color, path, cycles);
      }
    }
  }

  path.pop_back();
  color[current] = 2;

  return !cycles.empty();
}

// Build Resource Allocation Graph and Detect Cycles
void build_rag_and_detect_cycles(
    int num_proc, int num_res, const std::vector<std::vector<int>> &allocation,
    const std::vector<std::vector<int>> &request, std::vector<GraphEdge> &edges,
    std::vector<std::string> &cycles) {
  edges.clear();
  cycles.clear();

  std::map<std::string, std::vector<std::string>> adj;
  std::set<std::string> all_nodes;

  // Create Allocation Edges: Resource -> Process
  for (int i = 0; i < num_proc; i++) {
    std::string p_name = "P" + std::to_string(i);

    all_nodes.insert(p_name);

    for (int j = 0; j < num_res; j++) {
      std::string r_name = "R" + std::to_string(j);

      all_nodes.insert(r_name);

      if (allocation[i][j] > 0) {
        GraphEdge e;

        e.from_node = r_name;
        e.to_node = p_name;
        e.type = "ALLOCATION";

        edges.push_back(e);
        adj[r_name].push_back(p_name);
      }
    }
  }

  // Create Request Edges: Process -> Resource
  for (int i = 0; i < num_proc; i++) {
    std::string p_name = "P" + std::to_string(i);

    for (int j = 0; j < num_res; j++) {
      std::string r_name = "R" + std::to_string(j);

      if (request[i][j] > 0) {
        GraphEdge e;

        e.from_node = p_name;
        e.to_node = r_name;
        e.type = "REQUEST";

        edges.push_back(e);
        adj[p_name].push_back(r_name);
      }
    }
  }

  // Initialize Color Array
  std::map<std::string, int> color;

  for (const auto &node : all_nodes) {
    color[node] = 0;
  }

  // Run DFS Cycle Detection
  for (const auto &node : all_nodes) {
    if (color[node] == 0) {
      std::vector<std::string> path;

      dfs_rag_cycle(node, adj, color, path, cycles);
    }
  }
}

// Main Deadlock Detection Algorithm
DeadlockDetectionResult
run_deadlock_detection(const std::vector<std::vector<int>> &allocation,
                       const std::vector<std::vector<int>> &request,
                       const std::vector<int> &available,
                       bool is_single_instance) {
  DeadlockDetectionResult result;

  result.resource_model =
      is_single_instance ? "SINGLE_INSTANCE" : "MULTIPLE_INSTANCE";

  int num_proc = static_cast<int>(allocation.size());
  int num_res = static_cast<int>(available.size());

  // Build Resource Allocation Graph
  build_rag_and_detect_cycles(num_proc, num_res, allocation, request,
                              result.graph_edges, result.detected_cycles);

  // Initialize Work and Finish
  std::vector<int> work(num_res);
  std::vector<bool> finish(num_proc, false);

  for (int j = 0; j < num_res; j++) {
    work[j] = available[j];
  }

  for (int i = 0; i < num_proc; i++) {
    int has_allocation = 0;

    for (int j = 0; j < num_res; j++) {
      if (allocation[i][j] > 0) {
        has_allocation = 1;
        break;
      }
    }

    if (has_allocation == 0) {
      finish[i] = true;
    } else {
      finish[i] = false;
    }
  }

  int count = 0;
  int step_num = 1;
  int found = 0;

  // Deadlock Detection Algorithm
  while (1) {
    found = 0;

    for (int i = 0; i < num_proc; i++) {
      int can_finish = 1;

      if (finish[i] == false) {
        for (int j = 0; j < num_res; j++) {
          if (request[i][j] > work[j]) {
            can_finish = 0;
            break;
          }
        }

        if (can_finish == 1) {
          DetectionStep step;

          step.step_number = step_num++;
          step.process_id = i;
          step.work_before = work;
          step.request = request[i];
          step.allocation_reclaimed = allocation[i];

          // Release Allocated Resources
          for (int j = 0; j < num_res; j++) {
            work[j] += allocation[i][j];
          }

          finish[i] = true;
          count++;

          step.work_after = work;
          step.finish_state = finish;

          std::ostringstream ss;

          ss << "Process P" << i
             << " can complete because its Request is less than or equal to "
                "Work. "
             << "Its allocated resources are released back to Work.";

          step.reason = ss.str();

          result.steps.push_back(step);
          result.finished_processes.push_back(i);

          found = 1;
        }
      }
    }

    if (found == 0) {
      break;
    }
  }

  // Check for Deadlocked Processes
  result.deadlocked_processes.clear();

  for (int i = 0; i < num_proc; i++) {
    if (finish[i] == false) {
      result.deadlocked_processes.push_back(i);
    }
  }

  result.is_deadlocked = !result.deadlocked_processes.empty();

  // Display Detection Explanation
  std::ostringstream exp;

  if (result.is_deadlocked) {
    exp << "DEADLOCK DETECTED! Processes ";

    for (int i = 0; i < result.deadlocked_processes.size(); i++) {
      if (i > 0) {
        exp << ", ";
      }

      exp << "P" << result.deadlocked_processes[i];
    }

    exp << " are in a deadlock condition because their outstanding requests "
        << "cannot be fulfilled with the remaining available resources. ";
  } else {
    exp << "NO DEADLOCK DETECTED. All processes can successfully complete "
        << "with the available and reclaimed resources. ";
  }

  // Resource Model Explanation
  if (is_single_instance) {
    exp << "\n[Single-Instance Model]: In a single-instance resource system, "
        << "a directed cycle in the Resource Allocation Graph is necessary "
        << "and sufficient proof of deadlock. ";

    if (!result.detected_cycles.empty()) {
      exp << "Detected graph cycle(s): " << result.detected_cycles[0]
          << ". Hence deadlock is conclusively proven.";
    } else {
      exp << "No graph cycle found. Hence the system is free of deadlock.";
    }
  } else {
    exp << "\n[Multiple-Instance Model]: With multiple instances per resource "
        << "type, a graph cycle is not sufficient proof of deadlock. "
        << "The vector-based detection algorithm provides the determination.";
  }

  result.explanation = exp.str();

  return result;
}

} // namespace OSManager