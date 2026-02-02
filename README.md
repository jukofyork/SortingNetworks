# SortingNetworks

A high-performance C++20 implementation of beam search for discovering optimal sorting networks. This tool searches for sorting networks with minimal comparator count (length) and parallel execution time (depth) using heuristic-guided beam search with parallel evaluation.

## Overview

### What are Sorting Networks?

Sorting networks are fixed sequences of compare-exchange operations (comparators) that sort arbitrary inputs of a given size. Unlike comparison-based sorting algorithms like quicksort or mergesort, sorting networks have a predetermined structure that performs the same operations regardless of input values. This makes them particularly useful for hardware implementations, parallel processing, and scenarios where branching is expensive.

A compare-exchange operation between two wires (i, j) compares the values on those wires and swaps them if they are out of order. A sorting network is correct if it sorts all possible input permutations.

### The Optimization Problem

Finding optimal sorting networks is a classic combinatorial optimization problem. For an n-input network:

- There are n(n-1)/2 possible comparator positions
- The search space grows exponentially with n
- We seek networks with minimal length (total comparators) and minimal depth (parallel execution layers)

This implementation uses beam search with stochastic scoring to efficiently explore the vast search space.

### Target Bounds

The program searches for networks that match or improve upon the best known bounds documented at [https://bertdobbelaere.github.io/sorting_networks.html](https://bertdobbelaere.github.io/sorting_networks.html). The known optimal bounds are embedded in the code for networks from 2 to 32 inputs.

## Building

### Requirements

- GCC with C++20 support
- OpenMP
- Linux

### Build Commands

```bash
# Release build (optimized)
make

# Debug build (with symbols, no optimization)
make debug

# Clean build artifacts
make clean
```

## Usage

```bash
./sorting_networks [options]
```

### Command-Line Options

| Option | Long Form | Description | Default |
|--------|-----------|-------------|---------|
| `-i` | `--max-iterations` | Maximum search iterations | 1 |
| `-n` | `--net-size` | Network size (2-32) | 8 |
| `-b` | `--beam-size` | Beam width for search | 100 |
| `-t` | `--scoring-tests` | Number of scoring tests per state | 5 |
| `-e` | `--elite-tests` | Number of elite tests to average | 1 |
| `-s` | `--symmetry` | Enable symmetry heuristic | auto |
| `-S` | `--no-symmetry` | Disable symmetry heuristic | auto |
| `-z` | `--zobrist` | Enable Zobrist hashing for deduplication | off |
| `-w` | `--depth-weight` | Weight for depth vs length (0.0-1.0) | 0.0001 |
| `-h` | `--help` | Show help message | - |

### Parameter Guide

**Beam Size (`-b`)**: Controls the breadth of the search. Larger values explore more candidates per level but require more memory and computation. Values of 50-500 are typical for networks up to size 16.

**Scoring Tests (`-t`)**: Number of Monte Carlo simulations run to evaluate each candidate state. More tests provide better estimates but increase computation time. Values of 3-10 are typical.

**Elite Tests (`-e`)**: Number of best-scoring simulations to average when computing a state's final score. Must be less than or equal to scoring tests. Using 1-2 elites works well.

**Depth Weight (`-w`)**: Trade-off between optimizing for network length vs depth. Values near 0.0 prioritize shorter networks; values near 1.0 prioritize shallower networks. The default 0.0001 slightly prefers shorter networks.

**Zobrist Hashing (`-z`)**: Enables state deduplication using Zobrist hashing. This can significantly reduce redundant work when the symmetry heuristic is disabled. Has modest memory overhead.

### Symmetry Heuristic

The symmetry heuristic reduces the search space by exploiting symmetry properties of sorting networks. For even-sized networks, operations often come in symmetric pairs. By only considering one operation from each symmetric pair under certain conditions, the search space can be reduced.

By default:
- **Enabled** for even-sized networks (where symmetry is more effective)
- **Disabled** for odd-sized networks

Use `-s` or `-S` to explicitly override this default. The heuristic is most effective for larger networks.

### Examples

Search for an optimal 8-input network:
```bash
./sorting_networks -n 8
```

Search with larger beam and more thorough scoring:
```bash
./sorting_networks -n 12 -b 500 -t 10
```

Fast search with reduced parameters:
```bash
./sorting_networks -n 16 -b 50 -t 3 -e 1
```

Force symmetry heuristic for an odd-sized network:
```bash
./sorting_networks -n 17 -s
```

Optimize primarily for depth:
```bash
./sorting_networks -n 16 -w 0.8
```

Enable Zobrist hashing for deduplication:
```bash
./sorting_networks -n 8 -z
```

## Output Format

The program outputs configuration parameters followed by search progress and results:

```
MAX_ITERATIONS          = 1
NET_SIZE                = 8
MAX_BEAM_SIZE           = 100
NUM_SCORING_TESTS       = 5
NUM_ELITE_TESTS         = 1
USE_SYMMETRY_HEURISTIC  = Yes
USE_ZOBRIST_HASHING     = No
DEPTH_WEIGHT            = 0.0001
NUM_INPUT_PATTERNS      = 256
INPUT_PATTERN_TYPE      = uint8_t
LENGTH_LOWER_BOUND      = 19
LENGTH_UPPER_BOUND      = 38
DEPTH_LOWER_BOUND       = 6

Iteration 1:
0 [1] 1 [28] 2 3 4 5 6 7 8 9 10 11 12 13 14 15 16 17 18 19
+1:(0,2)
+2:(1,3)
+3:(4,6)
+4:(5,7)
...
+19:(3,4)
+Length: 19
+Depth : 6

Total Iterations  : 1
Total Time        : 0.247 seconds
```

### Output Fields

- **Iteration N:** Marks the start of a new search iteration
- **Level numbers (0, 1, 2...):** Current depth in the beam search
- **[N]:** Current beam size at that level
- **+N:(A,B):** The Nth comparator, operating between wires A and B
- **+Length:** Total number of comparators in the network
- **+Depth:** Number of parallel layers (network execution time)

## Algorithm

### Beam Search

The algorithm performs a breadth-first search with a fixed beam width (maximum number of candidates kept at each level):

1. **Initialization**: Start with an empty network (single candidate in beam)

2. **Expansion**: For each candidate in the current beam, generate all possible next comparators that would make progress (affect at least one unsorted input pattern)

3. **Scoring**: Evaluate each candidate using Monte Carlo simulation:
   - Run multiple random completions from the current state
   - Each completion randomly selects valid comparators until all input patterns are sorted
   - Calculate length and depth for each completion
   - Average the best (elite) completions to get a score

4. **Selection**: Keep the best-scoring candidates up to the beam width

5. **Termination**: Stop when a candidate has no valid successors (all input patterns are sorted)

6. **Post-processing**: Apply greedy depth minimization by reordering independent comparators

### State Representation

The algorithm tracks which input patterns remain unsorted using a compact bit representation:

- For an n-input network, there are 2^n possible binary input patterns
- Patterns are represented using the smallest unsigned integer type that fits (uint8_t for n≤8, uint16_t for n≤16, uint32_t for n≤32)
- An intrusive linked list tracks unsorted patterns for efficient iteration
- Zobrist hashing provides fast state comparison and deduplication

### Monte Carlo Scoring

Each candidate state is scored by simulating random completions:

```
score = (1 - depth_weight) * mean_length + depth_weight * mean_depth
```

#### Random Comparator Selection Optimization

The algorithm uses an efficient O(1) random selection strategy instead of the naive O(n²) approach:

**Naive approach (O(branching_factor)):**
1. Enumerate all n(n-1)/2 possible comparators
2. Check each for validity (would affect at least one unsorted pattern)
3. Collect all valid comparators into a list
4. Pick one randomly from the list

This requires O(n²) work per selection, which becomes expensive when performed thousands of times per scoring run.

**Optimized approach (O(num_unsorted) average case):**
1. Pick a random index from the unsorted pattern list (O(1))
2. Traverse the intrusive linked list to find that pattern (O(num_unsorted), typically small as unsorted patterns decrease rapidly)
3. Retrieve precomputed valid comparators for that specific pattern from lookup tables (O(1))
4. Pick a random comparator from this small set (O(1))

This optimization provides two benefits:
- **Performance**: Reduces selection from O(n²) to O(num_unsorted), which is typically much smaller
- **Bias**: By selecting a pattern first, comparators are naturally weighted by how many unsorted patterns they can affect. Operations that are valid for many patterns are more likely to be chosen, biasing the search toward comparators that make broad progress.

### Parallelization

The implementation uses OpenMP for parallel execution:

- **Candidate Collection**: All beam entries are processed in parallel to find valid successors
- **Scoring**: All candidate successors are scored in parallel
- **Thread-local Storage**: Each thread maintains its own random number generator and state buffers to avoid synchronization overhead

### Depth Minimization

After finding a valid network, a greedy algorithm attempts to minimize depth by reordering comparators:

1. Two comparators can execute in parallel if they operate on disjoint sets of wires
2. The algorithm greedily moves comparators earlier in the sequence when possible
3. This preserves correctness while reducing the number of parallel layers

## Supported Network Sizes

The program includes optimal bounds for networks from 2 to 32 inputs based on current research literature. Known optimal lengths and depths are:

| Size | Length | Depth |
|------|--------|-------|
| 2 | 1 | 1 |
| 4 | 5 | 3 |
| 8 | 19 | 6 |
| 16 | 60 | 9 |
| 32 | 185 | 14 |

The target upper bounds are automatically set based on these best known results. The search terminates early if it finds a network matching or improving upon these bounds.

## Implementation Details

- **Language**: C++20 with GCC
- **Parallelism**: OpenMP for multi-threaded evaluation
- **Memory**: Dynamic allocation scales with 2^n for n-input networks
- **Random Number Generation**: Thread-local Mersenne Twister with unique seeds per thread
- **State Deduplication**: Optional Zobrist hashing eliminates redundant states
- **Pattern Storage**: Template-selected integer type based on network size

## Performance Considerations

- **Memory Usage**: Dominated by the pattern lookup table (2^n entries). Networks larger than 20 inputs require significant memory.
- **Computation Time**: Scales with beam size, scoring iterations, and network size. Large networks (n>16) may require hours or days of search time.
- **Parallel Efficiency**: Near-linear speedup with core count for the scoring phase. Candidate collection has some synchronization overhead.

## References

- Best known sorting network bounds: https://bertdobbelaere.github.io/sorting_networks.html
- Sorting network theory and applications: Knuth, "The Art of Computer Programming, Vol. 3: Sorting and Searching"
- Beam search for combinatorial optimization: Original algorithm by Lowerre (1976)
