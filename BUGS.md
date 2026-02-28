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

## Invalid Optimizations (Do NOT Apply)

### OPT-001: Redundant Check in update_state() - **INVALID**

**Location:** `src/state.h:147`

**Status:** INVALID - Do not apply this optimization

**Why it's wrong:**
The `is_sorted()` check is necessary for the **NEW pattern** after applying the comparator transformation, not the old pattern. When we transform pattern A into pattern B by applying a comparator, pattern B might be sorted even though pattern A wasn't. The `in_list` check only tells us if pattern B was already in the list (duplicate), but doesn't tell us if pattern B is now sorted.

**If removed:**
- Sorted patterns can end up in `unsorted_patterns`
- `do_random_transition()` will try to select operations from an empty `allowed_ops` list
- Results in segmentation fault when accessing `allowed[rand_op]` on empty vector

**Conclusion:** The original code is correct. Both checks are necessary:
1. `in_list == 1` - checks if new pattern is a duplicate (already being tracked)
2. `is_sorted()` - checks if new pattern is now sorted (no operations needed)

---

## Fixed Bugs

*No bugs have been fixed yet.*
