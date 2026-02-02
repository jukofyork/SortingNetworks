#pragma once

#include "config.h"
#include "types.h"
#include <vector>
#include <cstdint>
#include <random>

// LookupTables precomputes information about all possible input patterns
// to speed up the search. For an n-wire network, there are 2^n possible
// binary input patterns.
class LookupTables {
public:
    // Initialize lookup tables based on network configuration.
    // Precomputes which patterns are already sorted and which compare-exchange
    // operations are valid for each pattern.
    // Conditionally initializes Zobrist hashing table based on enable_zobrist flag.
    void initialize(const Config& config, bool enable_zobrist = true) {
        const int n = config.get_net_size();
        const std::size_t num_patterns = config.get_num_input_patterns();

        is_sorted_.resize(num_patterns);
        allowed_ops_.resize(num_patterns);

        // Determine which input patterns are already sorted.
        // A pattern is sorted if all 0s come before all 1s (e.g., 0001111).
        for (std::size_t v = 0; v < num_patterns; ++v) {
            is_sorted_[v] = check_sorted(static_cast<int>(v), n) ? 1 : 0;
        }

        // For each unsorted pattern, precompute all valid compare-exchange operations.
        // An operation (i,j) is valid if the pattern has 0 at position i and 1 at position j.
        // This means applying the comparator would change the pattern.
        for (std::size_t i = 0; i < num_patterns; ++i) {
            allowed_ops_[i].clear();
            for (int n1 = 0; n1 < n - 1; ++n1) {
                for (int n2 = n1 + 1; n2 < n; ++n2) {
                    if (((static_cast<int>(i) >> n1) & 1) == 0 && ((static_cast<int>(i) >> n2) & 1) == 1) {
                        allowed_ops_[i].push_back(Operation{static_cast<std::uint8_t>(n1), static_cast<std::uint8_t>(n2)});
                    }
                }
            }
        }

        // Initialize Zobrist hash table only if enabled.
        // This avoids memory overhead when Zobrist hashing is not needed.
        has_zobrist_ = enable_zobrist;
        if (enable_zobrist) {
            zobrist_table_.resize(num_patterns);
            std::mt19937_64 rng(0xDEADBEEF);  // Fixed seed for reproducibility
            for (std::size_t i = 0; i < num_patterns; ++i) {
                zobrist_table_[i] = rng();
            }
        } else {
            zobrist_table_.clear();
        }
    }

    // Check if a given input pattern is already sorted.
    [[nodiscard]] bool is_sorted(int pattern) const { return is_sorted_[pattern] != 0; }

    // Get the list of valid compare-exchange operations for a pattern.
    // These are operations that would change the pattern (have 0 at op1, 1 at op2).
    [[nodiscard]] const std::vector<Operation>& allowed_ops(int pattern) const {
        return allowed_ops_[pattern];
    }

    // Get the number of valid operations for a pattern.
    [[nodiscard]] int num_allowed_ops(int pattern) const {
        return static_cast<int>(allowed_ops_[pattern].size());
    }

    // Check if Zobrist hashing is enabled.
    [[nodiscard]] bool has_zobrist() const {
        return has_zobrist_;
    }

    // Get the Zobrist hash value for a pattern.
    // Used for incremental state hashing in State class.
    [[nodiscard]] std::uint64_t zobrist_hash(int pattern) const {
        return has_zobrist_ ? zobrist_table_[pattern] : 0;
    }

private:
    // Bitmask indicating which patterns are already sorted.
    std::vector<std::uint8_t> is_sorted_;

    // For each pattern, stores the list of valid compare-exchange operations.
    // An operation is valid if it would actually change the pattern.
    std::vector<std::vector<Operation>> allowed_ops_;

    // Zobrist hash table: random 64-bit value for each pattern.
    // Used for incremental state hashing to detect duplicate states.
    std::vector<std::uint64_t> zobrist_table_;

    // Whether Zobrist hashing is enabled for this configuration.
    bool has_zobrist_ = false;

    // Check if a binary pattern represents a sorted sequence.
    // A pattern is sorted if all 0s appear before all 1s.
    // We check this by ensuring no 0 follows a 1 in the bit representation.
    [[nodiscard]] static bool check_sorted(int v, int n) {
        for (int i = 0; i < n - 1; ++i) {
            if (((v >> i) & 1) == 0 && ((v >> (i + 1)) & 1) == 1) {
                return false;
            }
        }
        return true;
    }
};
