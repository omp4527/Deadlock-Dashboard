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
