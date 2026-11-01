# Operating Systems IA Assignment Activity 2: Test Cases & Analytical Report

**Project Title:** OS Resource Manager — Deadlock Analysis & Resource Allocation Dashboard  
**Course:** CS304 Operating Systems  
**Module:** Deadlock Avoidance, Detection, Prevention, and Recovery  
**Core Implementation:** Native HTTP REST Backend  

---

## 1. Test Case Summary Matrix

| Test ID | Test Scenario | System Condition | Input Summary | Expected Result | Actual Result (Kernel Output) | Status |
| :--- | :--- | :--- | :--- | :--- | :--- | :---: |
| **TC1** | Safe State Verification | Baseline 5 processes, 3 resource types; Total = [10, 5, 7] | `Alloc`, `Max`, `Avail = [3, 3, 2]` | System is Safe; safe sequence exists | Safe (`<P1, P3, P4, P0, P2>`); 5 steps recorded | **PASS** |
| **TC2** | Request Exceeds Available | Baseline state; P0 Need = [7, 4, 3], Avail = [3, 3, 2] | P0 requests `[0, 4, 0]` | Status: WAITING; Allocation unmodified | Status: `WAITING`; State modified: `false` | **PASS** |
| **TC3** | Request Produces Unsafe State | Baseline state; P0 Need = [7, 4, 3], Avail = [3, 3, 2] | P0 requests `[0, 3, 0]` | Status: DENIED; Allocation rolled back | Status: `DENIED`; State modified: `false` | **PASS** |
| **TC4** | Deadlock Detection & Recovery | Multi-instance system; Avail = [0, 0, 0] | Circular requests in `Alloc` & `Request` | Deadlock detected in {P1, P2, P3, P4}; Terminate P2 to recover | Deadlock detected in P1, P2, P3, P4; Terminating P2 clears deadlock | **PASS** |
| **TC5** | Deadlock Prevention Policy | Fixed resource ordering: $R_0 < R_1 < R_2$ | P0 holds $R_1$, requests $R_0$; P1 holds $R_0$, requests $R_2$ | P0 blocked (violation); P1 allowed; No circular wait | P0 Violation flagged; P1 Compliant; Circular wait broken | **PASS** |
| **TC6** | Boundary & Invalid Inputs | Boundary conditions and invalid inputs tested individually | 1) Zero Avail, 2) Zero Need, 3) PID 99, 4) Req -1, 5) Alloc > Max | 1 & 2 valid boundary states; 3, 4, 5 rejected as invalid | 1 & 2 Valid; 3, 4, 5 Rejected with informative errors | **PASS** |

---

## 2. Detailed Test Case Analysis

### TC1 — Safe State Verification
- **Objective:** Verify Dijkstra’s Banker’s Algorithm safety check on a textbook 5-process, 3-resource system ($P_0 \dots P_4$, $A, B, C$).
- **Configuration:**
  - **Allocation Matrix:**
    $$P_0: [0, 1, 0], \quad P_1: [2, 0, 0], \quad P_2: [3, 0, 2], \quad P_3: [2, 1, 1], \quad P_4: [0, 0, 2]$$
  - **Maximum Demand Matrix:**
    $$P_0: [7, 5, 3], \quad P_1: [3, 2, 2], \quad P_2: [9, 0, 2], \quad P_3: [2, 2, 2], \quad P_4: [4, 3, 3]$$
  - **Available Vector:** $[3, 3, 2]$
- **Need Calculation:**
  $$\text{Need}[i][j] = \text{Maximum}[i][j] - \text{Allocation}[i][j]$$
  $$P_0: [7, 4, 3], \quad P_1: [1, 2, 2], \quad P_2: [6, 0, 0], \quad P_3: [0, 1, 1], \quad P_4: [4, 3, 1]$$
- **Trace Execution:**
  1. $\text{Work} = [3, 3, 2]$. $P_1$ satisfies $\text{Need}[1] \le \text{Work}$ ($[1, 2, 2] \le [3, 3, 2]$). $\text{Work} \leftarrow [3, 3, 2] + [2, 0, 0] = [5, 3, 2]$.
  2. $P_3$ satisfies $\text{Need}[3] \le \text{Work}$ ($[0, 1, 1] \le [5, 3, 2]$). $\text{Work} \leftarrow [5, 3, 2] + [2, 1, 1] = [7, 4, 3]$.
  3. $P_4$ satisfies $\text{Need}[4] \le \text{Work}$ ($[4, 3, 1] \le [7, 4, 3]$). $\text{Work} \leftarrow [7, 4, 3] + [0, 0, 2] = [7, 4, 5]$.
  4. $P_0$ satisfies $\text{Need}[0] \le \text{Work}$ ($[7, 4, 3] \le [7, 4, 5]$). $\text{Work} \leftarrow [7, 4, 5] + [0, 1, 0] = [7, 5, 5]$.
  5. $P_2$ satisfies $\text{Need}[2] \le \text{Work}$ ($[6, 0, 0] \le [7, 5, 5]$). $\text{Work} \leftarrow [7, 5, 5] + [3, 0, 2] = [10, 5, 7]$.
- **Conclusion:** All processes successfully completed with safe sequence $\langle P_1, P_3, P_4, P_0, P_2 \rangle$. The state is provably safe.

---

### TC2 — Request Exceeds Available Resources
- **Objective:** Verify Rule 2 of Banker's Resource-Request algorithm when a process requests resources within its declared maximum claim but in excess of current physical availability.
- **Input:** $P_0$ submits request vector $[0, 4, 0]$.
  - $\text{Need}[0] = [7, 4, 3] \implies [0, 4, 0] \le [7, 4, 3]$ (Rule 1 passes).
  - $\text{Available} = [3, 3, 2] \implies \text{Request}[1] = 4 > \text{Available}[1] = 3$ (Rule 2 triggers wait).
- **Kernel Backend Action:**
  - Status returned: `WAITING`.
  - Message: *"Request deferred: Insufficient resources currently available. Process P0 must wait."*
  - Allocation and Available vectors remain **completely unmodified**.
- **Conclusion:** Conforms strictly to Operating System theory; processes are blocked without state corruption.

---

### TC3 — Request Produces an Unsafe State
- **Objective:** Verify Rule 3 of Banker's Algorithm where resources are available, but speculative allocation leaves the system unable to guarantee completion without deadlock.
- **Input:** $P_0$ submits request vector $[0, 3, 0]$.
  - $\text{Need}[0] = [7, 4, 3] \implies [0, 3, 0] \le [7, 4, 3]$ (Passes).
  - $\text{Available} = [3, 3, 2] \implies [0, 3, 0] \le [3, 3, 2]$ (Passes).
- **Tentative Allocation & Safety Check:**
  - $\text{Available}_{\text{tentative}} = [3, 0, 2]$.
  - Checking Needs against Work $[3, 0, 2]$:
    - $P_0 \text{ Need} = [7, 1, 3]$ (fails on $B$: $1 > 0$)
    - $P_1 \text{ Need} = [1, 2, 2]$ (fails on $B$: $2 > 0$)
    - $P_2 \text{ Need} = [6, 0, 0]$ (fails on $A$: $6 > 3$)
    - $P_3 \text{ Need} = [0, 1, 1]$ (fails on $B$: $1 > 0$)
    - $P_4 \text{ Need} = [4, 3, 1]$ (fails on $B$: $3 > 0$)
  - Zero processes can finish! Tentative state is **UNSAFE**.
- **Kernel Backend Action:**
  - Status returned: `DENIED`.
  - Message: *"Request denied: Allocation would lead to an unsafe state."*
  - Automated state rollback executed. Live memory holds original allocation $[0, 1, 0]$ and available $[3, 3, 2]$.
- **Conclusion:** Demonstrates the core value of Banker's avoidance: preventing unsafe transitions before they cause system deadlocks.

---

### TC4 — Deadlock Detection & Recovery
- **Objective:** Demonstrate the multiple-instance vector detection algorithm (Silberschatz) and recovery by process abort.
- **Scenario:**
  - Allocation: $P_0:[0,1,0], P_1:[2,0,0], P_2:[3,0,3], P_3:[2,1,1], P_4:[0,0,2]$
  - Request: $P_0:[0,0,0], P_1:[2,0,2], P_2:[0,0,1], P_3:[1,0,0], P_4:[0,0,2]$
  - Available: $[0, 0, 0]$
- **Detection Algorithm Steps:**
  1. $\text{Work} = [0, 0, 0]$. $P_0$ has $\text{Request}[0] = [0, 0, 0] \le \text{Work}$. $P_0$ finishes. $\text{Work} \leftarrow [0, 1, 0]$.
  2. Remaining processes $P_1, P_2, P_3, P_4$ have requests needing resources $A$ and/or $C$, but Work has only $B=1$.
  3. No process can proceed.
  4. $\text{Finish} = [\text{true}, \text{false}, \text{false}, \text{false}, \text{false}]$.
  5. Deadlocked processes: $\{P_1, P_2, P_3, P_4\}$.
- **Recovery Action:**
  - Abort process $P_2$, reclaiming held resources $[3, 0, 3]$.
  - Updated Available becomes $[3, 1, 3]$.
  - Re-running detection: $P_1$ requests $[2, 0, 2] \le [3, 1, 3]$, completes and frees resources, enabling $P_3$ and $P_4$ to complete.
- **Conclusion:** Successfully identified deadlocked set and restored deadlock-free execution via targeted recovery.

---

### TC5 — Deadlock Prevention via Global Resource Ordering
- **Objective:** Eliminate Circular Wait by enforcing Havender’s hierarchical resource acquisition policy: $R_0 < R_1 < R_2$.
- **Scenario:**
  - $P_0$ holds $R_1$ (rank 1), requests $R_0$ (rank 0).
  - $P_1$ holds $R_0$ (rank 0), requests $R_2$ (rank 2).
- **Evaluation:**
  - $P_0$ request: $\text{rank}(R_0) = 0 \le \text{rank}(R_1) = 1 \implies$ **POLICY VIOLATION**. Request blocked.
  - $P_1$ request: $\text{rank}(R_2) = 2 > \text{rank}(R_0) = 0 \implies$ **COMPLIANT**. Request granted.
- **Mathematical Invariant:**
  - In any circular wait $P_0 \rightarrow P_1 \rightarrow \dots \rightarrow P_k \rightarrow P_0$, the resource ranks along the path must satisfy:
    $$\text{rank}(r_0) < \text{rank}(r_1) < \dots < \text{rank}(r_k) < \text{rank}(r_0)$$
  - This strict inequality is impossible for integers. Circular wait is eliminated.
- **Conclusion:** The policy prevents deadlock statically without runtime safety-check overhead.

---

### TC6 — Boundary & Invalid Input Handling
- **Objective:** Validate robust system boundary behavior and rejection of invalid data.
- **Sub-Tests:**
  1. **Zero Available Resources:** $\text{Available} = [0, 0, 0]$. Recognized as a **VALID boundary state** (e.g. all resources currently allocated).
  2. **Process with Zero Remaining Need:** $\text{Need} = [0, 0, 0]$. Recognized as a **VALID boundary state** (process is immediately eligible to finish and release resources).
  3. **Invalid Process Identifier:** Request sent for $P_{99}$. Rejected with code 400: *"Invalid process identifier P99. Must be between P0 and P4."*
  4. **Negative Resource Request:** Request vector $[-1, 0, 0]$. Rejected: *"Resource request values must be non-negative integers."*
  5. **Allocation Exceeding Maximum Claim:** Allocation $= [8, 0, 0]$, Maximum $= [7, 0, 0]$. Rejected: *"Allocation cannot exceed declared Maximum demand."*
- **Conclusion:** Clear distinction between legal boundary states and structural syntax violations.
