# SortingNetworks

A C++20 implementation of beam search for discovering minimum-length sorting networks. This tool searches for optimal sorting networks using heuristic-guided beam search with parallel evaluation.

## Overview

Sorting networks are fixed sequences of compare-exchange operations that sort arbitrary inputs. Finding optimal networks (with minimal comparators and depth) is a classic combinatorial optimization problem. This implementation uses beam search with stochastic scoring to explore the vast search space efficiently.

The program searches for networks that match or improve upon the best known bounds documented at [https://bertdobbelaere.github.io/sorting_networks.html](https://bertdobbelaere.github.io/sorting_networks.html).

## Building

Requirements:
- GCC with C++20 support
- OpenMP
- Linux

```bash
make
```

For debug build:
```bash
make debug
```

## Usage

```bash
./sorting_networks [options]
```

### Command-Line Options

| Option | Long Form | Description | Default |
|--------|-----------|-------------|---------|
| `-n` | `--net-size` | Network size (2-32) | 16 |
| `-b` | `--beam-size` | Beam width for search | 1000 |
| `-t` | `--test-runs` | Random test runs per state evaluation | 10 |
| `-e` | `--elites` | Number of elite samples to average | 1 |
| `-w` | `--depth-weight` | Weight for depth vs length (0.0-1.0) | 0.0001 |
| `-a` | `--asymmetry` | Enable asymmetry heuristic | auto |
| `-A` | `--no-asymmetry` | Disable asymmetry heuristic | auto |
| `-i` | `--max-iterations` | Maximum search iterations | 1000000 |
| `-h` | `--help` | Show help message | - |

### Asymmetry Heuristic

The asymmetry heuristic reduces the search space by exploiting symmetry properties. By default:
- **Enabled** for even-sized networks
- **Disabled** for odd-sized networks

Use `-a` or `-A` to explicitly override this default.

### Examples

Search for an optimal 8-input network:
```bash
./sorting_networks -n 8
```

Search with reduced beam width and fewer test runs (faster but less thorough):
```bash
./sorting_networks -n 12 -b 500 -t 5
```

Force asymmetry heuristic for an odd-sized network:
```bash
./sorting_networks -n 17 -a
```

Optimize for depth rather than length:
```bash
./sorting_networks -n 16 -w 0.9
```

## Output Format

The program outputs discovered networks in the following format:

```
NET_SIZE                = 8
MAX_BEAM_SIZE           = 1000
NUM_TEST_RUNS           = 10
NUM_TEST_RUN_ELITES     = 1
USE_ASYMMETRY_HEURISTIC = Yes
DEPTH_WEIGHT            = 0.0001
LENGTH_UPPER_BOUND      = 19
DEPTH_UPPER_BOUND       = 6

Iteration 1:
0 [1] 1 [28] 2 ... 18 19 
+1:(0,2)
+2:(1,3)
...
+19:(3,4)
+Length: 19
+Depth : 6
```

Each comparator is shown as `+N:(A,B)` where N is the step number and (A,B) represents a compare-exchange operation between wires A and B. The final length and depth are reported, with depth representing the number of parallel layers.

## Algorithm

The search uses beam search with the following features:

1. **Stochastic Scoring**: Each state is evaluated by running multiple random simulations to estimate the remaining comparators needed
2. **Elite Selection**: The best-scoring samples are averaged to determine state priority
3. **Asymmetry Reduction**: Optional heuristic that exploits network symmetries to prune the search space
4. **Parallel Evaluation**: OpenMP parallelization for successor state evaluation
5. **Depth Minimization**: Post-processing step to minimize parallel depth by reordering independent comparators

## Supported Network Sizes

The program includes optimal bounds for networks from 2 to 32 inputs based on current research literature. The target upper bounds are automatically set based on the best known results.

## Implementation Details

- **Language**: C++20
- **Parallelism**: OpenMP for multi-threaded successor evaluation
- **Memory**: Dynamic allocation based on runtime configuration
- **RNG**: Thread-local Mersenne Twister for reproducible parallel behavior

## License

[Add your license here]

## References

Best known sorting network bounds: https://bertdobbelaere.github.io/sorting_networks.html

Algorithm based on beam search techniques for combinatorial optimization with heuristic guidance.
