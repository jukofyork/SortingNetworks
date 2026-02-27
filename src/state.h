#pragma once

#include "config.h"
#include "lookup.h"
#include "types.h"
#include <vector>
#include <memory>
#include <algorithm>
#include <iostream>
#include <random>
#include <thread>
#include <cstdint>

// State represents the current progress of sorting network construction.
// It tracks which input patterns have been sorted and which operations have been applied.
template<int NetSize>
class State {
public:
    using PatternType = typename BitStorage<NetSize>::type;

    // ListElement represents a node in the intrusive linked list of unsorted patterns.
    // This is a space-efficient way to track which input patterns still need sorting.
    struct ListElement {
        std::uint8_t in_list;      // 1 if this pattern is currently in the unsorted list
        PatternType bit_pattern;   // The binary pattern value
        int next;                  // Index of next element in linked list, -1 if end
    };

    // Linked list tracking unsorted input patterns.
    // Uses intrusive linked list stored in used_list for O(1) removal/insertion.
    std::vector<ListElement> used_list;
    int first_used = -1;       // Index of first unsorted pattern in linked list
    int num_unsorted = 0;      // Number of patterns still needing to be sorted

    // Sequence of compare-exchange operations applied so far.
    std::vector<Operation> operations;
    int current_level = 0;     // Current number of operations in the sequence

    explicit State(const Config& config);
    State(const State& other) = default;
    State& operator=(const State& other) = default;

    // Reset state to initial condition with all non-trivial unsorted patterns.
    // Trivial patterns (all 0s, single 1, etc.) are already sorted by definition.
    void set_start_state(const Config& config, const LookupTables& lookups);

    // Apply a compare-exchange operation to all unsorted patterns.
    // This updates the linked list by moving patterns to their new values
    // or removing them if they become sorted.
    void update_state(int op1, int op2, const LookupTables& lookups);

    // Select a random unsorted pattern and apply a random valid operation to it.
    // By picking a random unsorted pattern first, we weight operations by how many
    // patterns they can affect. Operations valid for many patterns are more likely chosen.
    void do_random_transition(const LookupTables& lookups);

    void print_state() const;

    // Greedy algorithm to minimize parallel depth by reordering operations.
    // Attempts to maximize parallelism while preserving correctness.
    void minimise_depth(int net_size);

    // Calculate the parallel depth (number of parallel layers) of the network.
    // Two operations can be in the same layer if they use disjoint sets of wires.
    [[nodiscard]] int get_depth(int net_size) const;

    // Score this state using fixed number of Monte Carlo simulations.
    // Runs exactly num_tests simulations and returns the mean score.
    [[nodiscard]] double score_state(int num_tests, double depth_weight, const LookupTables& lookups);

    // Find all valid successor operations from current state.
    // An operation is valid if it would change at least one unsorted pattern.
    [[nodiscard]] int find_successors(std::vector<std::vector<int>>& succ_ops, int net_size);

private:
    // Thread-local random number generator for parallel execution.
    static std::mt19937& get_thread_rng() {
        thread_local std::mt19937 rng([]() {
            std::random_device rd;
            auto tid = std::hash<std::thread::id>{}(std::this_thread::get_id());
            return static_cast<unsigned>(rd() + tid);
        }());
        return rng;
    }

    static int rand_int(int n) {
        if (n <= 0) return 0;
        std::uniform_int_distribution<int> dist(0, n);
        return dist(get_thread_rng());
    }
};

template<int NetSize>
State<NetSize>::State(const Config& config) {
    used_list.resize(config.get_num_input_patterns());
    operations.resize(config.get_length_upper_bound());
}

// Initialize the state with all non-trivial unsorted patterns.
// Patterns with 0 or 1 ones are trivially sorted, so we start with patterns
// that have at least 2 ones (indicating potential unsortedness).
template<int NetSize>
void State<NetSize>::set_start_state(const Config& config, const LookupTables& lookups) {
    first_used = -1;

    for (std::size_t i = 0; i < config.get_num_input_patterns(); ++i) {
        if (lookups.is_sorted(i)) {
            used_list[i].in_list = 0;
        } else {
            used_list[i].in_list = 1;
            used_list[i].next = first_used;
            used_list[i].bit_pattern = static_cast<PatternType>(i);
            first_used = i;
        }
    }

    // Subtract (n+1) for the n+1 trivial sorted patterns (0 ones, 1 one, ..., n ones but sorted)
    num_unsorted = config.get_num_input_patterns() - (config.get_net_size() + 1);
    current_level = 0;
}

// Apply a compare-exchange operation between wires op1 and op2.
// For each unsorted pattern, if it has 0 at op1 and 1 at op2, we swap them.
// This is done by updating the bit pattern and managing the linked list.
template<int NetSize>
void State<NetSize>::update_state(int op1, int op2, const LookupTables& lookups) {
    int last_index = -1;

    for (int used_index = first_used; used_index != -1; ) {
        int next_index = used_list[used_index].next;
        PatternType pattern = used_list[used_index].bit_pattern;

        // Check if this pattern would be affected by the comparator (op1, op2)
        if (((pattern >> op1) & 1) == 0 && ((pattern >> op2) & 1) == 1) {
            // Remove old pattern from tracking
            used_list[static_cast<std::size_t>(pattern)].in_list = 0;

            // Apply the comparator: set bit at op1, clear bit at op2
            pattern |= (static_cast<PatternType>(1) << op1);
            pattern &= (~(static_cast<PatternType>(1) << op2));

            // Check if the new pattern is now sorted
            if (used_list[static_cast<std::size_t>(pattern)].in_list == 1 || lookups.is_sorted(static_cast<int>(pattern))) {
                num_unsorted--;
                if (last_index != -1)
                    used_list[last_index].next = next_index;
                else
                    first_used = next_index;
                // New pattern is sorted, so it leaves the set
            } else {
                // New pattern needs further sorting, keep in list
                used_list[static_cast<std::size_t>(pattern)].in_list = 1;
                used_list[used_index].bit_pattern = pattern;
                if (last_index != -1)
                    used_list[last_index].next = used_index;
                else
                    first_used = used_index;
                last_index = used_index;
            }
        } else {
            last_index = used_index;
        }

        used_index = next_index;
    }

    operations[current_level].op1 = static_cast<std::uint8_t>(op1);
    operations[current_level].op2 = static_cast<std::uint8_t>(op2);
    current_level++;
}

// Select a random unsorted pattern and apply a random valid operation.
// By picking a random unsorted pattern first, we weight operations by how many
// patterns they can affect. Operations valid for many patterns are more likely chosen.
template<int NetSize>
void State<NetSize>::do_random_transition(const LookupTables& lookups) {
    int rand_index = rand_int(num_unsorted - 1);
    int true_index = 0;
    int n = 0;

    for (int i = first_used; i != -1; i = used_list[i].next) {
        if (n == rand_index) {
            true_index = static_cast<int>(used_list[i].bit_pattern);
            break;
        }
        n++;
    }

    const auto& allowed = lookups.allowed_ops(true_index);
    int rand_op = rand_int(static_cast<int>(allowed.size()) - 1);
    update_state(allowed[rand_op].op1, allowed[rand_op].op2, lookups);
}

template<int NetSize>
void State<NetSize>::print_state() const {
    for (const auto& elem : used_list) {
        std::cout << elem.bit_pattern << ':' << static_cast<int>(elem.in_list) << std::endl;
    }
    std::cout << "First used: " << first_used << std::endl;
    std::cout << "Unsorted: " << num_unsorted << std::endl;
}

// Greedy depth minimization by reordering independent operations.
// Two operations are independent (can be parallel) if they use different wires.
// This algorithm greedily moves operations earlier when possible to reduce depth.
template<int NetSize>
void State<NetSize>::minimise_depth(int net_size) {
    bool altered;

    do {
        altered = false;
        std::vector<int> used_ops1(net_size, 0);
        std::vector<int> used_ops2(net_size, 0);

        for (int l1 = 0; l1 < current_level; ++l1) {
            if (used_ops1[operations[l1].op1] == 1 || used_ops1[operations[l1].op2] == 1) {
                std::fill(used_ops2.begin(), used_ops2.end(), 0);

                for (int l2 = l1; l2 < current_level; ++l2) {
                    if (used_ops2[operations[l2].op1] == 1 || used_ops2[operations[l2].op2] == 1)
                        break;

                    if (used_ops1[operations[l2].op1] != 1 && used_ops1[operations[l2].op2] != 1) {
                        used_ops1[operations[l2].op1] = 1;
                        used_ops1[operations[l2].op2] = 1;

                        std::swap(operations[l1], operations[l2]);

                        l2 = l1;
                        l1++;
                        std::fill(used_ops2.begin(), used_ops2.end(), 0);
                        altered = true;
                        continue;
                    }

                    used_ops2[operations[l2].op1] = 1;
                    used_ops2[operations[l2].op2] = 1;
                }

                std::fill(used_ops1.begin(), used_ops1.end(), 0);
            }

            used_ops1[operations[l1].op1] = 1;
            used_ops1[operations[l1].op2] = 1;
        }
    } while (altered);
}

// Calculate parallel depth by counting how many layers are needed.
// A new layer starts when an operation shares a wire with a previous operation
// in the current layer.
template<int NetSize>
int State<NetSize>::get_depth(int net_size) const {
    std::vector<int> used_ops(net_size, 0);
    int num_used = 1;

    for (int i = 0; i < current_level; ++i) {
        if (used_ops[operations[i].op1] == 1 || used_ops[operations[i].op2] == 1) {
            std::fill(used_ops.begin(), used_ops.end(), 0);
            num_used++;
        }

        used_ops[operations[i].op1] = 1;
        used_ops[operations[i].op2] = 1;
    }

    return num_used;
}

// Score a state using fixed number of Monte Carlo simulations.
// Runs exactly num_tests simulations and returns the mean score.
template<int NetSize>
double State<NetSize>::score_state(int num_tests, double depth_weight, const LookupTables& lookups) {
    double total_score = 0.0;
    State<NetSize> temp_state(*this);

    for (int test = 0; test < num_tests; ++test) {
        temp_state = *this;

        // Complete the network with random operations
        while (temp_state.num_unsorted > 0) {
            temp_state.do_random_transition(lookups);
        }

        temp_state.minimise_depth(NetSize);

        double length = temp_state.current_level;
        double depth = temp_state.get_depth(NetSize);
        double score = (1.0 - depth_weight) * length + depth_weight * depth;
        total_score += score;
    }

    return total_score / num_tests;
}

// Find all valid successor operations from the current state.
// An operation is valid if it would change at least one unsorted pattern.
// Returns the number of valid successors found.
template<int NetSize>
int State<NetSize>::find_successors(std::vector<std::vector<int>>& succ_ops, int net_size) {
    int allowed = 0;

    for (auto& row : succ_ops) {
        std::fill(row.begin(), row.end(), 0);
    }

    // Check all unsorted patterns to see which operations would affect them
    for (int i = first_used; i != -1; i = used_list[i].next) {
        PatternType pattern = used_list[i].bit_pattern;
        for (int n1 = 0; n1 < net_size - 1; ++n1) {
            for (int n2 = n1 + 1; n2 < net_size; ++n2) {
                if (((pattern >> n1) & 1) == 0 && ((pattern >> n2) & 1) == 1) {
                    succ_ops[n1][n2] = 1;
                }
            }
        }
    }

    // Count total valid operations
    for (int n1 = 0; n1 < net_size - 1; ++n1) {
        for (int n2 = n1 + 1; n2 < net_size; ++n2) {
            if (succ_ops[n1][n2] == 1) {
                allowed++;
            }
        }
    }

    return allowed;
}
