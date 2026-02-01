// state.h
// =======
// Modern C++20 state management for sorting network beam search.
// Now uses runtime-sized vectors.

#pragma once

#include "config.h"
#include "lookup.h"
#include <vector>
#include <array>
#include <memory>
#include <algorithm>
#include <iostream>
#include <random>
#include <thread>
#include <ctime>
#include <cstdint>

namespace sorting_networks {

// ============================================================================
// List Element Structure
// ============================================================================

struct ListElement {
    unsigned in_list : 1;    // 1=Yes, 0=No
    unsigned vector : 31;    // Vector (& Vector index) held
    int next;                // Index to next in list (-1 == End)
};

// ============================================================================
// State Class
// ============================================================================

class State {
public:
    // Data members - now dynamically sized
    std::vector<ListElement> used_list;
    int first_used = -1;
    int num_unsorted = 0;
    
    std::vector<std::array<byte, 2>> operations;
    int current_level = 0;

    // Constructor - initializes vectors to correct sizes
    State() {
        const int tests = g_config.num_tests;
        const int ops = g_config.max_ops;
        used_list.resize(tests);
        operations.resize(ops);
    }
    
    // Copy constructor
    State(const State& other) 
        : used_list(other.used_list),
          first_used(other.first_used),
          num_unsorted(other.num_unsorted),
          operations(other.operations),
          current_level(other.current_level) {}
    
    // Copy assignment
    State& operator=(const State& other) {
        if (this != &other) {
            used_list = other.used_list;
            first_used = other.first_used;
            num_unsorted = other.num_unsorted;
            operations = other.operations;
            current_level = other.current_level;
        }
        return *this;
    }

    // Core methods
    void set_start_state();
    void update_state(int op1, int op2);
    void do_random_transition();
    void print_state() const;
    void minimise_depth();
    [[nodiscard]] int get_depth() const;
    [[nodiscard]] double score_state();
    [[nodiscard]] int find_successors(std::vector<std::vector<int>>& succ_ops);
    
private:
    // Thread-local random number generator
    static std::mt19937& get_thread_rng() {
        thread_local std::mt19937 rng([]() {
            auto tid = std::hash<std::thread::id>{}(std::this_thread::get_id());
            auto time_seed = static_cast<std::uint32_t>(std::time(nullptr));
            return tid + time_seed;
        }());
        return rng;
    }
    
    // Generate random integer in [0, n]
    static int rand_int(int n) {
        if (n <= 0) return 0;
        std::uniform_int_distribution<int> dist(0, n);
        return dist(get_thread_rng());
    }
};

// ============================================================================
// Implementation
// ============================================================================

inline void State::set_start_state() {
    first_used = -1;
    const int tests = g_config.num_tests;
    const int n = g_config.net_size;
    
    for (int i = 0; i < tests; ++i) {
        if (g_lookups.is_sorted(i)) {
            used_list[i].in_list = 0;
        } else {
            used_list[i].in_list = 1;
            used_list[i].next = first_used;
            used_list[i].vector = i;
            first_used = i;
        }
    }
    
    num_unsorted = tests - (n + 1);
    current_level = 0;
}

inline void State::update_state(int op1, int op2) {
    int last_index = -1;
    
    for (int used_index = first_used; used_index != -1; ) {
        int next_index = used_list[used_index].next;
        int vector = used_list[used_index].vector;
        
        if (((vector >> op1) & 1) == 0 && ((vector >> op2) & 1) == 1) {
            used_list[vector].in_list = 0;
            vector |= (1 << op1);
            vector &= (~(1 << op2));
            
            if (used_list[vector].in_list == 1 || g_lookups.is_sorted(vector)) {
                num_unsorted--;
                if (last_index != -1)
                    used_list[last_index].next = next_index;
                else
                    first_used = next_index;
            } else {
                used_list[vector].in_list = 1;
                used_list[used_index].vector = vector;
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
    
    operations[current_level][0] = static_cast<byte>(op1);
    operations[current_level][1] = static_cast<byte>(op2);
    current_level++;
    
    if (current_level > g_config.max_ops) {
        std::cout << "Error: max_ops exceeded" << std::endl;
        std::exit(1);
    }
}

inline void State::do_random_transition() {
    int rand_index = State::rand_int(num_unsorted - 1);
    int true_index = 0;
    int n = 0;
    
    for (int i = first_used; i != -1; i = used_list[i].next) {
        if (n == rand_index) {
            true_index = used_list[i].vector;
            break;
        }
        n++;
    }
    
    const auto& allowed = g_lookups.allowed_ops(true_index);
    int rand_op = State::rand_int(static_cast<int>(allowed.size()) - 1);
    update_state(allowed[rand_op][0], allowed[rand_op][1]);
}

inline void State::print_state() const {
    const int tests = g_config.num_tests;
    for (int i = 0; i < tests; ++i) {
        std::cout << i << ':' << used_list[i].in_list << std::endl;
    }
    for (int i = 0; i < tests; ++i) {
        std::cout << i << ':' << used_list[i].vector << " =>" << used_list[i].next << std::endl;
    }
    std::cout << first_used << std::endl;
    std::cout << "Number Unsorted:" << num_unsorted << std::endl << std::endl;
}

inline void State::minimise_depth() {
    const int net_size = g_config.net_size;
    bool altered;
    
    do {
        altered = false;
        std::vector<int> used_ops1(net_size, 0);
        std::vector<int> used_ops2(net_size, 0);
        
        for (int l1 = 0; l1 < current_level; ++l1) {
            if (used_ops1[operations[l1][0]] == 1 || used_ops1[operations[l1][1]] == 1) {
                std::fill(used_ops2.begin(), used_ops2.end(), 0);
                
                for (int l2 = l1; l2 < current_level; ++l2) {
                    if (used_ops2[operations[l2][0]] == 1 || used_ops2[operations[l2][1]] == 1)
                        break;
                    
                    if (used_ops1[operations[l2][0]] != 1 && used_ops1[operations[l2][1]] != 1) {
                        used_ops1[operations[l2][0]] = 1;
                        used_ops1[operations[l2][1]] = 1;
                        
                        std::swap(operations[l1], operations[l2]);
                        
                        l2 = l1;
                        l1++;
                        std::fill(used_ops2.begin(), used_ops2.end(), 0);
                        altered = true;
                        continue;
                    }
                    
                    used_ops2[operations[l2][0]] = 1;
                    used_ops2[operations[l2][1]] = 1;
                }
                
                std::fill(used_ops1.begin(), used_ops1.end(), 0);
            }
            
            used_ops1[operations[l1][0]] = 1;
            used_ops1[operations[l1][1]] = 1;
        }
    } while (altered);
}

inline int State::get_depth() const {
    const int net_size = g_config.net_size;
    std::vector<int> used_ops(net_size, 0);
    int num_used = 1;
    
    for (int i = 0; i < current_level; ++i) {
        if (used_ops[operations[i][0]] == 1 || used_ops[operations[i][1]] == 1) {
            std::fill(used_ops.begin(), used_ops.end(), 0);
            num_used++;
        }
        
        used_ops[operations[i][0]] = 1;
        used_ops[operations[i][1]] = 1;
    }
    
    return num_used;
}

inline double State::score_state() {
    const int num_test_runs = g_config.num_test_runs;
    const int num_test_run_elites = g_config.num_test_run_elites;
    const double depth_weight = g_config.depth_weight;
    
    std::vector<int> levels(num_test_runs);
    std::vector<int> depths(num_test_runs);
    
    for (int test = 0; test < num_test_runs; ++test) {
        auto temp_state = std::make_unique<State>(*this);
        
        while (temp_state->num_unsorted > 0) {
            temp_state->do_random_transition();
        }
        
        temp_state->minimise_depth();
        
        levels[test] = temp_state->current_level;
        depths[test] = temp_state->get_depth();
    }
    
    // Sort using std::sort
    std::vector<std::pair<int, int>> results;
    results.reserve(num_test_runs);
    for (int i = 0; i < num_test_runs; ++i) {
        results.emplace_back(levels[i], depths[i]);
    }
    
    std::sort(results.begin(), results.end(), [depth_weight](const auto& a, const auto& b) {
        if (depth_weight < 0.5) {
            return a.first < b.first || (a.first == b.first && a.second < b.second);
        } else {
            return a.second < b.second || (a.second == b.second && a.first < b.first);
        }
    });
    
    double length_sum = 0;
    double depth_sum = 0;
    for (int test = 0; test < num_test_run_elites; ++test) {
        length_sum += results[test].first;
        depth_sum += results[test].second;
    }
    
    double mean_length = length_sum / num_test_run_elites;
    double mean_depth = depth_sum / num_test_run_elites;
    
    return ((1.0 - depth_weight) * mean_length) + (depth_weight * mean_depth);
}

inline int State::find_successors(std::vector<std::vector<int>>& succ_ops) {
    const int net_size = g_config.net_size;
    int allowed = 0;
    
    for (auto& row : succ_ops) {
        std::fill(row.begin(), row.end(), 0);
    }
    
    for (int i = first_used; i != -1; i = used_list[i].next) {
        for (int n1 = 0; n1 < net_size - 1; ++n1) {
            for (int n2 = n1 + 1; n2 < net_size; ++n2) {
                if (((used_list[i].vector >> n1) & 1) == 0 && ((used_list[i].vector >> n2) & 1) == 1) {
                    succ_ops[n1][n2] = 1;
                }
            }
        }
    }
    
    for (int n1 = 0; n1 < net_size - 1; ++n1) {
        for (int n2 = n1 + 1; n2 < net_size; ++n2) {
            if (succ_ops[n1][n2] == 1) {
                allowed++;
            }
        }
    }
    
    return allowed;
}

} // namespace sorting_networks
