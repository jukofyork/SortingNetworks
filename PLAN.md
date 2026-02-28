# SortingNetworks Codebase Refactoring Plan

## Executive Summary

This plan prioritizes **safety and performance preservation** while improving code readability and maintainability. The codebase is already well-structured, but has specific pain points that can be addressed without harming the critical paths.

---

## Phase 1: Zero-Risk Foundations (Week 1)

### 1.1 Add Named Constants (HIGH PRIORITY - ZERO RISK)

**Files to modify:**
- `src/types.h` (new constants file or add to existing)
- `src/state.h`
- `src/normalization.h`

**Changes:**

```cpp
// In types.h or new constants.h
#pragma once
#include <cstdint>

inline constexpr int END_OF_LIST = -1;
inline constexpr std::uint8_t INVALID_LABEL = 255;
inline constexpr int MAX_NET_SIZE = 32;
inline constexpr int NUM_NET_SIZE_CASES = 31;  // 2 through 32
```

**Replace magic numbers:**
- `state.h:32,104,129,148,157,181,308` - Replace `-1` with `END_OF_LIST`
- `normalization.h:37,48,62,63` - Replace `32` with `MAX_NET_SIZE`
- `normalization.h:64` - Replace `255` with `INVALID_LABEL`

**Performance Impact:** None - compile-time constants

---

### 1.2 Add Performance Benchmarks (CRITICAL - MUST DO FIRST)

**New file:** `src/benchmark.cpp`

```cpp
#include "state.h"
#include "search.h"
#include <chrono>
#include <iostream>

template<int NetSize>
void benchmark_score_state(const Config& config, const LookupTables& lookups) {
    State<NetSize> state(config);
    state.set_start_state(config, lookups);
    
    // Warmup
    for (int i = 0; i < 100; ++i) {
        state.score_state(5, 0.0001, lookups);
    }
    
    auto start = std::chrono::steady_clock::now();
    const int iterations = 10'000;
    for (int i = 0; i < iterations; ++i) {
        state.score_state(5, 0.0001, lookups);
    }
    auto end = std::chrono::steady_clock::now();
    
    double elapsed = std::chrono::duration<double>(end - start).count();
    std::cout << NetSize << ": " << (iterations / elapsed) << " calls/sec\n";
}

// Similar benchmarks for update_state, do_random_transition
```

**Acceptance Criteria:** Each subsequent phase must show <2% regression vs baseline

---

### 1.3 Fix rand_int Range Bug (HIGH PRIORITY)

**File:** `src/state.h:86-90`

**Bug:** `uniform_int_distribution<int>(0, n)` generates [0, n] inclusive (n+1 values), not n values.

**Fix:**

```cpp
// Returns random int in range [0, n-1] inclusive (n possible values)
// n is the count of items to select from
static int rand_int(int n) {
    if (n <= 0) return 0;
    std::uniform_int_distribution<int> dist(0, n - 1);
    return dist(get_thread_rng());
}
```

**Impact:** Fixes biased random selection and potential out-of-bounds access in `do_random_transition()`

---

## Phase 2: Safe Structural Improvements (Week 2)

### 2.1 Add Inline Documentation for Critical Data Structures (ZERO RISK)

**File:** `src/state.h:29-33`

Add comprehensive comment:

```cpp
// Intrusive linked list tracking unsorted input patterns
// INVARIANT: unsorted_patterns[i].in_list == 1  <=>  pattern i is currently unsorted
// INVARIANT: first_used == END_OF_LIST  <=>  num_unsorted == 0
// INVARIANT: num_unsorted == count of nodes in linked list
// PERFORMANCE: Stored in contiguous vector for cache efficiency
// NOTE: Name is historical; contains ONLY unsorted patterns despite "used" name
std::vector<ListElement> unsorted_patterns;
int first_used = END_OF_LIST;
int num_unsorted = 0;
```

**Rationale:** Clarifies design without risking rename-induced bugs

---

### 2.2 Extract Helper for Operation Sequence Building (LOW RISK)

**File:** `src/search.h`

Add after `CandidateSuccessor` definition:

```cpp
template<int NetSize>
[[gnu::always_inline]] inline
std::uint64_t build_operation_sequence(std::vector<Operation>& ops,
                                        const std::vector<Operation>& beam_ops,
                                        int level,
                                        std::uint8_t new_op1,
                                        std::uint8_t new_op2,
                                        int net_size) {
    ops.clear();
    ops.reserve(level + 1);
    for (int j = 0; j < level; ++j) {
        ops.push_back(beam_ops[j]);
    }
    ops.push_back(Operation{new_op1, new_op2});
    return compute_canonical_hash<NetSize>(ops, level + 1, net_size);
}
```

**Replace duplicate code at lines 176-187 and 197-209:**

```cpp
// Symmetry case (line ~176)
std::uint64_t hash = build_operation_sequence<NetSize>(
    thread_ops, beam[i], level, 
    static_cast<std::uint8_t>(inv_n1), 
    static_cast<std::uint8_t>(inv_n2), 
    net_size);

// Normal case (line ~197)  
std::uint64_t hash = build_operation_sequence<NetSize>(
    thread_ops, beam[i], level, 
    static_cast<std::uint8_t>(n1), 
    static_cast<std::uint8_t>(n2), 
    net_size);
```

**Performance Impact:** Minimal - [[gnu::always_inline]] forces inlining

---

### 2.3 Extract Candidate-to-Successor Copying (LOW RISK)

**File:** `src/search.h`

Add private method to `BeamSearchContext`:

```cpp
inline void copy_candidates_to_successors(
    std::vector<StateSuccessor>& successors,
    const std::vector<CandidateSuccessor>& candidates,
    const std::vector<double>& scores,  // empty if no scores
    size_t count) {
    
    successors.resize(count);
    for (size_t i = 0; i < count; ++i) {
        successors[i].beam_index = candidates[i].beam_index;
        successors[i].operation.op1 = candidates[i].op1;
        successors[i].operation.op2 = candidates[i].op2;
        successors[i].score = scores.empty() ? 0.0 : scores[i];
    }
}
```

**Replace lines 346-352 and 357-361:**

```cpp
// After successive halving (line ~346)
copy_candidates_to_successors(beam_successors, candidates, candidate_scores, 
                              current_beam_size);

// Without halving (line ~357)
copy_candidates_to_successors(beam_successors, candidates, {}, 
                              current_beam_size);
```

---

## Phase 3: Major Restructuring (Week 3-4) - AFTER BENCHMARKS PASS

### 3.1 Split beam_search() into Phases (MEDIUM RISK - MEASURE FIRST)

**File:** `src/search.h`

**Goal:** Reduce 290-line method to ~80-line orchestrator

**New private methods:**

```cpp
template<int NetSize>
[[gnu::flatten]] inline
int collect_candidates_parallel(
    std::vector<CandidateSuccessor>& candidates,
    int level,
    const Config& config,
    const LookupTables& lookups);

template<int NetSize>
inline void deduplicate_candidates(
    std::vector<CandidateSuccessor>& candidates);

template<int NetSize>
[[gnu::flatten]] inline
void select_best_candidates(
    const std::vector<CandidateSuccessor>& candidates,
    std::vector<StateSuccessor>& successors,
    int level,
    const Config& config,
    const LookupTables& lookups);

inline void rebuild_beam(int level);
```

**Implementation notes:**
- Mark parallel-containing methods with `[[gnu::flatten]]` to encourage inlining
- Keep `thread_local` declarations inside parallel regions, not at method scope
- Preserve `schedule(dynamic)` or add chunk size tuning

**Testing:** Must show <2% regression in benchmark suite

---

### 3.2 Add Aggressive Inlining Hints to Hot Functions (LOW RISK)

**File:** `src/state.h`

Mark performance-critical methods:

```cpp
template<int NetSize>
class State {
public:
    [[gnu::always_inline]] inline void update_state(
        std::uint8_t op1, std::uint8_t op2, 
        const LookupTables& lookups);
    
    [[gnu::always_inline]] inline void do_random_transition(
        const LookupTables& lookups);
    
    [[gnu::flatten]] inline double score_state(
        int num_tests, double depth_weight, 
        const LookupTables& lookups);
    
    // minimize_depth and get_depth can use [[gnu::flatten]]
};
```

---

## What NOT to Do (High Risk, Low Reward)

### DONE: Rename `used_list` -> `unsorted_patterns`

**Why:** 
- Touches 8+ locations across hot loops
- High risk of missing references during rename
- Zero functional benefit (already documented)

**Alternative:** Add clarifying comment (see Phase 2.1)

---

### DO NOT: Extract `NetworkOptimizer` Class

**Why:**
- `minimise_depth()` and `get_depth()` are called from hottest loop (`score_state`)
- Moving to another class prevents cross-method inlining
- Current template structure allows full optimization

**Alternative:** Keep as private methods in State, mark with `[[gnu::flatten]]`

---

### DO NOT: Extract `MonteCarloScorer` Class

**Why:**
- `score_state()` contains the tightest loop in entire program
- State copy `temp_state = *this` needs to stay optimizable
- Cross-template extraction adds complexity without benefit

**Alternative:** Keep in State, extract only the pure score calculation:

```cpp
[[gnu::always_inline]] inline
double calculate_score(double length, double depth, double depth_weight) {
    return (1.0 - depth_weight) * length + depth_weight * depth;
}
```

---

### DO NOT: Modernize C-style Loops Arbitrarily

**Skip these "improvements":**
- Replacing explicit loops with `std::iota` (may not optimize as well)
- Replacing explicit loops with `std::copy` (current code is clear)
- Using range-based for loops where index is needed

**Why:** Current code is explicit and optimizer-friendly. "Modern" alternatives add risk without clear benefit.

---

## Testing & Validation Protocol

### Before Each Phase:

1. **Baseline measurement:**
   ```bash
   make clean && make release
   ./benchmark_baseline > baseline.txt
   timeout 60 ./sorting_networks -n 10 -b 200 > baseline_result.txt
   ```

2. **Apply refactoring**

3. **Validation:**
   ```bash
   make clean && make release
   ./benchmark_baseline > after.txt
   # Must be within 2% of baseline
   timeout 60 ./sorting_networks -n 10 -b 200 > after_result.txt
   # Solution quality must match baseline
   ```

4. **Reject if:** >2% regression or different solution quality

---

## Summary of Changes by File

| File                | Changes                                                | Risk Level |
| ------------------- | ------------------------------------------------------ | ---------- |
| `types.h`             | Add constants (END_OF_LIST, MAX_NET_SIZE, etc.)        | Zero       |
| `state.h`             | Add documentation, inline hints, clarify rand_int      | Very Low   |
| `normalization.h`     | Replace magic 32 with MAX_NET_SIZE                     | Very Low   |
| `search.h`            | Extract 2 helper functions, split beam_search() phases | Low-Medium |
| `benchmark.cpp`       | New file for performance validation                    | Zero       |
| `SortingNetworks.cpp` | No changes needed                                      | -          |
| `config.cpp`          | No changes needed                                      | -          |
| `lookup.h`            | No changes needed                                      | -          |

---

## Expected Outcomes

**Readability Improvements:**
- Clear documentation of intrusive linked list design
- Named constants instead of magic numbers
- 290-line `beam_search()` reduced to ~80 lines
- Helper functions with clear single responsibilities

**Performance Preservation:**
- All critical paths (`do_random_transition`, `score_state`, `update_state`) remain inline
- OpenMP parallel regions preserved
- Benchmark suite ensures no regressions

**Maintainability:**
- Easier to understand beam search algorithm phases
- Clear separation of concerns in search.h
- Documented invariants for critical data structures

---

## Implementation Order

1. Phase 1.1: Add constants and benchmarks (do this first!)
2. Phase 1.3: Fix rand_int range bug
3. Phase 2.1: Document unsorted_patterns design
4. Run benchmarks, establish baseline
5. Phase 2.2: Extract build_operation_sequence helper
6. Phase 2.3: Extract copy_candidates_to_successors helper
7. Run benchmarks, verify no regression
8. Phase 3.1: Split beam_search into phases (if benchmarks pass)
9. Phase 3.2: Add inlining hints
10. Final benchmark validation

---

*This plan was generated through comprehensive analysis of the codebase using multiple explore agents to ensure safety and performance preservation.*
