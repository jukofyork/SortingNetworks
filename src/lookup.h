// lookup.h
// ========
// Modern C++20 lookup tables for sorting network search.
// Now uses runtime-sized vectors.

#pragma once

#include "config.h"
#include <vector>
#include <array>

namespace sorting_networks {

// ============================================================================
// Lookup Tables
// ============================================================================

class LookupTables {
public:
    void initialize() {
        const int n = g_config.net_size;
        const int tests = g_config.num_tests;
        
        // Resize vectors to runtime-computed sizes
        is_sorted_.resize(tests);
        allowed_ops_.resize(tests);
        
        // Create is_sorted lookup
        for (int v = 0; v < tests; ++v) {
            is_sorted_[v] = check_sorted(v, n);
        }
        
        // Create allowed_ops lookup
        for (int i = 0; i < tests; ++i) {
            allowed_ops_[i].clear();
            for (int n1 = 0; n1 < n - 1; ++n1) {
                for (int n2 = n1 + 1; n2 < n; ++n2) {
                    if (((i >> n1) & 1) == 0 && ((i >> n2) & 1) == 1) {
                        allowed_ops_[i].push_back(std::array<byte, 2>{static_cast<byte>(n1), static_cast<byte>(n2)});
                    }
                }
            }
        }
    }
    
    [[nodiscard]] bool is_sorted(int vector) const { return is_sorted_[vector]; }
    
    [[nodiscard]] const std::vector<std::array<byte, 2>>& allowed_ops(int vector) const {
        return allowed_ops_[vector];
    }
    
    [[nodiscard]] int num_allowed_ops(int vector) const {
        return static_cast<int>(allowed_ops_[vector].size());
    }

private:
    std::vector<bool> is_sorted_;
    std::vector<std::vector<std::array<byte, 2>>> allowed_ops_;
    
    [[nodiscard]] static bool check_sorted(int v, int n) {
        for (int i = 0; i < n - 1; ++i) {
            if (((v >> i) & 1) == 0 && ((v >> (i + 1)) & 1) == 1) {
                return false;
            }
        }
        return true;
    }
};

// Global instance
inline LookupTables g_lookups;

} // namespace sorting_networks
