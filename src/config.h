#pragma once

#include <cstdint>
#include <stdexcept>
#include <string>

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

class Config {
public:
    Config() = default;

    void initialize();
    void parse_args(int argc, char* argv[]);
    void print_usage(const char* program_name) const;
    void print() const;

    // Getters for user-configurable parameters
    [[nodiscard]] int get_max_iterations() const { return max_iterations_; }
    [[nodiscard]] int get_net_size() const { return net_size_; }
    [[nodiscard]] int get_max_beam_size() const { return max_beam_size_; }
    [[nodiscard]] int get_num_scoring_iterations() const { return num_scoring_iterations_; }
    [[nodiscard]] bool get_use_symmetry_heuristic() const { return use_symmetry_heuristic_; }
    [[nodiscard]] double get_depth_weight() const { return depth_weight_; }

    // Getters for computed parameters
    [[nodiscard]] std::size_t get_num_input_patterns() const { return num_input_patterns_; }
    [[nodiscard]] const char* get_input_pattern_type() const { return input_pattern_type_; }
    [[nodiscard]] int get_length_lower_bound() const { return length_lower_bound_; }
    [[nodiscard]] int get_length_upper_bound() const { return length_upper_bound_; }
    [[nodiscard]] int get_depth_lower_bound() const { return depth_lower_bound_; }
    [[nodiscard]] int get_branching_factor() const { return branching_factor_; }

private:
    // User-configurable parameters (with defaults)
    int max_iterations_ = 1;
    int net_size_ = 8;
    int max_beam_size_ = 100;
    int num_scoring_iterations_ = 5;
    bool use_symmetry_heuristic_ = true;
    bool symmetry_explicitly_set_ = false;
    double depth_weight_ = 0.0001;

    // Computed parameters
    std::size_t num_input_patterns_ = 0;
    const char* input_pattern_type_ = "uint8_t";
    int length_lower_bound_ = 0;
    int length_upper_bound_ = 0;
    int depth_lower_bound_ = 0;
    int branching_factor_ = 0;
};
