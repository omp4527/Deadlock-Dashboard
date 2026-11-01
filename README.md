# OS Resource Manager — Deadlock Analysis & Resource Allocation Dashboard

**Operating Systems Internal Assessment (IA) — Activity 2**  
*A complete, student-friendly, full-stack educational dashboard for Banker's Algorithm, Deadlock Detection, Prevention, and Recovery.*

---

## 1. Project Architecture & Technologies

This project strictly adheres to the requested technology stack:

- **Frontend:** Semantic HTML5, Vanilla CSS3 (custom dark navy sidebar + responsive cards), Vanilla JavaScript (ES6+). Zero third-party frontend frameworks (no React, Angular, Vue, or Streamlit).
- **Core Algorithms:** Native implementation of Dijkstra's Banker's Algorithm, Multi-Instance Vector Detection, Single-Instance Resource Allocation Graph (RAG) cycle detection, Havender's Hierarchical Prevention, and Termination Recovery.
- **Backend Bridge:** Native HTTP REST Server utilizing Windows Winsock (`ws2_32.dll`), serving both REST JSON API endpoints and static frontend assets with zero external package manager overhead.
- **Communication:** Standard REST API with JSON payloads over HTTP port `8080` (with cross-origin resource sharing / CORS enabled).
- **Visualizations:** HTML dynamic matrix tables, animated sequence chips, and SVG-generated Resource-Allocation Graphs with cycle highlighting.

```
DeadLock_Dashboard_UI/
├── backend/
│   ├── banker.cpp              # Banker's Need, Safety algorithm, Request handler
│   ├── banker.h                # Banker declarations & data structures
│   ├── deadlock_detection.cpp  # Multi-instance detection & RAG cycle analysis
│   ├── deadlock_detection.h    # Detection declarations & graph structures
│   ├── http_server.h           # Zero-dependency WinSock HTTP REST server
│   ├── json_helper.h           # Zero-dependency JSON parser & serializer
│   ├── main.cpp                # REST route registration & server entry
│   ├── prevention_recovery.cpp # Havender's ordering policy & process abort
│   ├── prevention_recovery.h   # Prevention/Recovery headers
│   ├── test_runner.cpp         # Automated runner for TC1 through TC6
│   ├── test_runner.h           # Test suite structures
│   ├── validation.cpp          # Input validation, non-negativity, invariant totals
│   └── validation.h            # Validation structures
├── frontend/
│   ├── app.js                  # Vanilla JS frontend state & API orchestration
│   ├── index.html              # 7-section responsive dashboard layout
│   └── style.css               # Navy dark sidebar + modern card styling
├── tests/
│   └── test_cases.md           # Full academic test case documentation
├── build.bat                   # 1-click Windows compilation script
├── run.bat                     # 1-click execution & browser launch script
└── README.md                   # Complete documentation, demo script & Viva Q&A
```

---

## 2. Setup and Execution Instructions (Windows & VS Code)

### Prerequisites
1. **Windows 10 or 11**.
2. **GCC/G++ Compiler** (MinGW, MinGW-w64, or TDM-GCC). Verify in terminal:
   ```powershell
   g++ --version
   ```
3. **VS Code** (Optional, recommended for development).

### Method A: One-Click Launch (Easiest)
1. Double-click `run.bat` (or execute `.\run.bat` in PowerShell/Command Prompt).
2. The script will automatically compile `server.exe` (if not already built), start the backend on port `8080`, and launch your default browser to `http://localhost:8080`.

### Method B: Manual Command Line
1. Open terminal inside the project directory:
   ```powershell
   cd DeadLock_Dashboard_UI
   ```
2. Compile the backend:
   ```powershell
   g++ -std=c++17 backend/*.cpp -lws2_32 -o server.exe
   ```
3. Start the server:
   ```powershell
   .\server.exe 8080
   ```
4. Open your browser and navigate to:
   ```
   http://localhost:8080
   ```

*(Note: If you prefer using VS Code Live Server on port 5500, you can open `frontend/index.html` directly with Live Server; the backend supports CORS headers and will respond seamlessly to API requests).*

---

## 3. Documented REST API Endpoints

All requests and responses use `application/json` format:

| Method | Endpoint | Description | Sample Request / Response |
| :--- | :--- | :--- | :--- |
| `GET` | `/api/health` | Backend status check | **Response:** `{"status": "ok", "engine": "Native Kernel"}` |
| `POST` | `/api/validate` | Validates allocation, maximum, and available | **Body:** `{"num_processes": 5, "num_resources": 3, "allocation": [...], "maximum": [...], "available": [...]}` |
| `POST` | `/api/banker/need` | Calculates Need matrix | **Response:** `{"success": true, "need": [[7,4,3], [1,2,2], ...]}` |
| `POST` | `/api/banker/safety` | Runs Dijkstra's Safety check | **Response:** `{"success": true, "safety": {"is_safe": true, "safe_sequence": [1,3,4,0,2], "trace": [...]}}` |
| `POST` | `/api/banker/request`| Simulates resource request | **Body:** `{"process_id": 0, "request": [0, 3, 0], ...}`<br>**Response:** `{"status": "DENIED", "state_modified": false}` |
| `POST` | `/api/deadlock/detect`| Multiple/Single instance detection | **Body:** `{"allocation": [...], "request": [...], "available": [...], "is_single_instance": false}` |
| `POST` | `/api/prevention/simulate`| Tests strict hierarchical ordering | **Body:** `{"allocation": [...], "requests": [...], "resource_order": [0, 1, 2]}` |
| `POST` | `/api/recovery/terminate`| Simulates process abort & recovery | **Body:** `{"process_id": 2, "allocation": [...], "request": [...], "available": [...]}` |
| `GET` | `/api/run-tests` | Runs TC1 through TC6 natively | **Response:** `{"total_tests": 6, "passed_tests": 6, "all_passed": true, "tests": [...]}` |

---

## 4. Analytical Results Table (TC1 to TC6)

Results verified directly from the native backend:

| Test ID | Test Scenario | System Condition | Expected Result | Actual Result (Kernel Output) | Pass/Fail | Analysis |
| :--- | :--- | :--- | :--- | :--- | :---: | :--- |
| **TC1** | Safe State Verification | Baseline 5-process, 3-resource system | Safe state exists with complete safe sequence | `State: SAFE, Sequence: <P1, P3, P4, P0, P2>` | **PASS** | Dijkstra's algorithm confirmed Work $[3, 3, 2]$ can satisfy all process needs sequentially without deadlock. |
| **TC2** | Request Exceeds Available | Baseline state; P0 requests $[0, 4, 0]$ | Status: WAITING; Allocation remains unmodified | `Status: WAITING, Allocation modified: false` | **PASS** | Request exceeds Available ($4 > 3$) while within Need. Process must wait; system state preserved. |
| **TC3** | Request Produces Unsafe State | Baseline state; P0 requests $[0, 3, 0]$ | Status: DENIED; Allocation rolled back | `Status: DENIED, Allocation modified: false` | **PASS** | Tentative allocation leaves Available $[3, 0, 2]$, failing all remaining needs. Request denied to prevent deadlock. |
| **TC4** | Deadlock Detection & Recovery | Multi-instance system; Available $[0, 0, 0]$ | Deadlock in {P1, P2, P3, P4}; Terminate P2 to recover | `Detection: DEADLOCKED (P1, P2, P3, P4). Terminated P2: can proceed = true` | **PASS** | Multi-instance vector detection verified deadlock; terminating P2 released $[3, 0, 3]$ and resolved circular wait. |
| **TC5** | Deadlock Prevention Policy | Fixed resource order: $R_0 < R_1 < R_2$ | P0 request blocked (violation); P1 request allowed | `Policy: true, P0 Complies: FALSE, P1 Complies: TRUE` | **PASS** | Strictly prevents Circular Wait by disallowing descending resource index requests without prior release. |
| **TC6** | Boundary & Invalid Inputs | Individual boundary and erroneous inputs | 1 & 2 valid boundary; 3, 4, 5 rejected | `Zero Avail: Valid, Zero Need: Valid, Invalid PID: Rejected, Neg: Rejected, Alloc>Max: Rejected` | **PASS** | Successfully differentiates valid mathematical edge conditions from illegal structural inputs. |

---

## 5. Six-Minute Demonstration Script

Use this script during your laboratory demonstration or video submission:

### Minute 1: Introduction & Architecture (0:00 – 1:00)
- *"Good morning/afternoon. Today I am presenting the OS Resource Manager Dashboard developed for IA Activity 2."*
- *"Notice that the frontend is written purely in Vanilla HTML5, CSS3, and JavaScript, with no external frameworks. The core algorithms for the Banker’s Algorithm, Deadlock Detection, Prevention, and Recovery are implemented entirely natively."*
- *"Look at the top sidebar indicator: the native WinSock server is running on port 8080 and communicating via REST JSON endpoints."*

### Minute 2: Dashboard Overview & Setup (1:00 – 2:00)
- *"On the Overview page, we have four real-time metric cards showing 5 Active Processes, 3 Resource Types, Available Vector $[3, 3, 2]$, and current Safe State status."*
- *"Switching to Section 2 (Allocation Setup), we dynamically configure processes and resource types. Notice the resource accounting invariant: $\text{Total}[j] = \sum \text{Allocation}[i][j] + \text{Available}[j] = [10, 5, 7]$."*
- *"When I click 'Calculate Need', the backend immediately computes $\text{Need} = \text{Maximum} - \text{Allocation}$."*

### Minute 3: Banker's Algorithm Safe State (2:00 – 3:00)
- *"Navigating to Section 3 (Banker’s Algorithm), we observe Dijkstra's Safety Algorithm."*
- *"The system displays the safe sequence $\langle P_1, P_3, P_4, P_0, P_2 \rangle$."*
- *"In the trace table below, each step clearly displays Work Before, Process Need, the conditional check $\text{Need} \le \text{Work}$, reclaimed allocation, Work After, and the boolean Finish vector."*

### Minute 4: Resource Request Simulator (3:00 – 4:00)
- *"Moving to Section 4 (Request Simulator), let's test dynamic requests."*
- *"First, click preset 'Exceeds Avail (P0: 0,4,0)'. Process $P_0$ requests 4 instances of B, but only 3 are available. The kernel immediately flags this as **WAITING / Deferred**, preserving the allocation untouched."*
- *"Next, click preset 'Unsafe Request (P0: 0,3,0)'. Here, resources are physically available, but tentative allocation leaves Work at $[3, 0, 2]$, which satisfies no process. The backend rejects this as **DENIED**, safely rolling back the state."*

### Minute 5: Deadlock Detection, Prevention & Recovery (4:00 – 5:00)
- *"In Section 5 (Deadlock Detection), we examine outstanding requests separate from Banker's algorithm. Clicking 'Run Detection' executes the vector detection algorithm, revealing deadlocked processes $P_1, P_2, P_3, P_4$. The interactive SVG Resource Allocation Graph visually displays the allocation and request edges."*
- *"In Section 6 (Prevention & Recovery), we demonstrate Havender’s strict ordering ($R_0 < R_1 < R_2$). $P_0$ holding $R_1$ and requesting $R_0$ is rejected as a policy violation, mathematically breaking Circular Wait."*
- *"Under Recovery, aborting $P_2$ releases $[3, 0, 3]$, instantly clearing the deadlock."*

### Minute 6: Test Cases & Execution (5:00 – 6:00)
- *"In Section 7 (Test Cases & Results), we trigger `/api/run-tests`. All six required test cases (TC1 through TC6) execute and pass with a 100% green bar in our Analytical Results Table."*
- *"Thank you. I am ready for viva questions."*

---

## 6. Viva Voce Questions & Answers

### Q1: What is the core difference between Deadlock Avoidance (Banker's) and Deadlock Detection?
**Answer:**
- **Deadlock Avoidance** is proactive. It requires prior knowledge of each process's **Maximum Demand**. Before granting any resource request, it simulates the allocation to ensure the system remains in a **Safe State** where at least one execution sequence exists that avoids circular waiting.
- **Deadlock Detection** is reactive. It does not require maximum demand claims. It examines the current system state (current Allocations, Available resources, and current outstanding Requests) to discover whether processes are **already** in a circular wait condition.

### Q2: What is the exact distinction between an "Unsafe State" and a "Deadlocked State"?
**Answer:**
- An **Unsafe State** is *not* necessarily a deadlocked state. Rather, an unsafe state is one from which the operating system can no longer *guarantee* that deadlock will not happen if all processes suddenly request their maximum declared demand.
- A **Deadlocked State** is an actual runtime condition where two or more processes are permanently blocked because each is holding resources that the other needs, and neither can proceed. All deadlocked states are unsafe, but not all unsafe states are deadlocked.

### Q3: Why does TC2 return "Waiting" while TC3 returns "Denied"?
**Answer:**
- In **TC2**, the request $\le \text{Need}$, but the request exceeds the currently **Available** resources. This is a normal operational delay; the process is placed in a waiting queue until earlier processes release resources. The state is valid, just deferred.
- In **TC3**, the requested resources are physically available, but granting them would leave the remaining available pool so depleted that no process could satisfy its remaining need. The request is **Denied** because granting it would transition the system into an unsafe state. The speculative allocation is cancelled and rolled back.

### Q4: In a Resource Allocation Graph (RAG), does the existence of a cycle always mean deadlock?
**Answer:**
- **Single-Instance Resources:** Yes. A directed cycle in a single-instance RAG is both **necessary and sufficient** proof of deadlock.
- **Multiple-Instance Resources:** No. A cycle is a **necessary condition, but NOT sufficient**. If other processes outside the cycle hold instances of the resource type involved in the cycle and can finish, they will release their instances, which breaks the cycle and allows the processes to proceed.

### Q5: How does Havender's Hierarchical Resource Ordering mathematically prevent Deadlock?
**Answer:**
- Deadlock requires four simultaneous conditions: Mutual Exclusion, Hold and Wait, No Preemption, and Circular Wait.
- Havender's policy targets **Circular Wait**. Every resource type is assigned a strict integer rank: $F(R_i) \in \mathbb{N}$. A process may only request resource $R_j$ if $F(R_j) > F(R_{\text{held}})$.
- In any hypothetical circular wait cycle $P_0 \rightarrow P_1 \rightarrow \dots \rightarrow P_k \rightarrow P_0$, the rank of requested resources would have to strictly increase along the cycle: $F(R_0) < F(R_1) < \dots < F(R_k) < F(R_0)$. This implies $F(R_0) < F(R_0)$, which is mathematically impossible for integers. Hence, circular wait is eliminated.

### Q6: What trade-offs arise when enforcing Deadlock Prevention?
**Answer:**
1. **Reduced Concurrency:** Processes cannot request resources as they become logically required by program flow; they must request them in artificial numerical order.
2. **Resource Under-utilization:** A process may have to acquire resources much earlier than needed or hold them idle to satisfy ranking rules.
3. **Starvation Potential:** Processes needing multiple high-ranked and low-ranked resources may wait longer if preceding allocations are delayed.

### Q7: Explain the significance of Test Case 6 (TC6). Why is Available = [0, 0, 0] not considered an input error?
**Answer:**
- An Available vector of $[0, 0, 0]$ simply means that all physical instances of resources are currently allocated to active processes. This is a completely legal, valid **boundary state**. If a process currently holds resources and has a remaining $\text{Need} = [0, 0, 0]$, it can finish immediately and replenish Available.
- In contrast, negative requests ($[-1, 0, 0]$), non-existent process IDs ($P_{99}$), and allocations exceeding maximum claims are structural **input violations** that violate basic OS consistency invariants. TC6 proves that our system distinguishes between physical boundary limits and semantic data errors.
