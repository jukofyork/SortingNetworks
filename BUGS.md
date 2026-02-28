# Known Bugs and Issues

## Active Bugs

### BUG-001: rand_int() Range Off-by-One Error (HIGH PRIORITY)

**Location:** `src/state.h:88`

**Description:**
The `rand_int()` function uses `std::uniform_int_distribution<int>(0, n)` which generates values in the range [0, n] inclusive - that's n+1 possible values, not n values.

**Current Code:**
```cpp
static int rand_int(int n) {
    if (n <= 0) return 0;
    std::uniform_int_distribution<int> dist(0, n);  // BUG: [0, n] = n+1 values
    return dist(get_thread_rng());
}
```

**Impact:**
- Random selection is biased
- May cause out-of-bounds access when selecting from arrays/vectors
- Affects `do_random_transition()` at lines 177 and 190

**Fix:**
```cpp
static int rand_int(int n) {
    if (n <= 0) return 0;
    std::uniform_int_distribution<int> dist(0, n - 1);  // FIXED: [0, n-1] = n values
    return dist(get_thread_rng());
}
```

**Status:** Pending fix in Phase 1.3 of PLAN.md

---

## Optimization Opportunities

### OPT-001: Redundant Check in update_state()

**Location:** `src/state.h:143`

**Description:**
The condition checks both `used_list[pattern].in_list == 1` and `lookups.is_sorted(pattern)`, but the first check already implies the pattern is unsorted.

**Current Code:**
```cpp
if (used_list[static_cast<std::size_t>(pattern)].in_list == 1 || 
    lookups.is_sorted(static_cast<int>(pattern))) {
```

**Rationale for Simplification:**
- Patterns are only added to `used_list` if `!is_sorted()` (see `set_start_state()`)
- If `in_list == 1`, the pattern is already known to be unsorted
- The `is_sorted()` check is redundant

**Suggested Fix:**
```cpp
if (used_list[static_cast<std::size_t>(pattern)].in_list == 1) {
```

**Impact:** Minor - removes one function call in hot path

**Status:** Optional optimization for future refactoring

---

## Fixed Bugs

*No bugs have been fixed yet.*
