// config.h
// ========
// Runtime configuration for sorting network search.
// Modern C++20 version - supports command-line configuration.

#pragma once

#include <cstdint>
#include <algorithm>
#include <stdexcept>
#include <string>

namespace sorting_networks {

// ============================================================================
// Type Aliases
// ============================================================================

using byte = std::uint8_t;
using word = std::uint16_t;

// ============================================================================
// Bounds Lookup
// ============================================================================

struct Bounds {
    int length;
    int depth;
};

// See: https://bertdobbelaere.github.io/sorting_networks.html
constexpr Bounds get_bounds(int n) {
    switch (n) {
        case 2:  return {1, 1};
        case 3:  return {3, 3};
        case 4:  return {5, 3};
        case 5:  return {9, 5};
        case 6:  return {12, 5};
        case 7:  return {16, 6};
        case 8:  return {19, 6};
        case 9:  return {25, 7};
        case 10: return {29, 7};
        case 11: return {35, 8};
        case 12: return {39, 8};
        case 13: return {45, 9};
        case 14: return {51, 9};
        case 15: return {56, 9};
        case 16: return {60, 9};
        case 17: return {71, 10};
        case 18: return {77, 11};
        case 19: return {85, 11};
        case 20: return {91, 11};
        case 21: return {99, 12};
        case 22: return {106, 12};
        case 23: return {114, 12};
        case 24: return {120, 12};
        case 25: return {130, 13};
        case 26: return {138, 13};
        case 27: return {147, 13};
        case 28: return {155, 13};
        case 29: return {164, 14};
        case 30: return {172, 14};
        case 31: return {180, 14};
        case 32: return {185, 14};
        default: return {0, 0};
    }
}

// ============================================================================
// Runtime Configuration
// ============================================================================

class Config {
public:
    // Network parameters
    int net_size = 16;
    int max_beam_size = 1000;
    int num_test_runs = 10;
    int num_test_run_elites = 1;
    double depth_weight = 0.0001;
    bool use_asymmetry_heuristic = true;
    int max_iterations = 1'000'000;
    bool asymmetry_explicitly_set = false;  // Track if user overrode default
    
    // Computed values
    int branching_factor = 0;
    int num_tests = 0;
    int length_upper_bound = 0;
    int depth_upper_bound = 0;
    int max_ops = 0;
    
    // Initialize computed values based on net_size
    // Also sets default asymmetry heuristic: on for even, off for odd
    void initialize();
    
    // Parse command line arguments
    void parse_args(int argc, char* argv[]);
    
    // Print usage information
    static void print_usage(const char* program_name);
    
    // Print current configuration
    void print() const;
};

// Global config instance
inline Config g_config;

} // namespace sorting_networks
