// search.h
// ========
// Modern C++20 beam search implementation.
// Now uses runtime-sized vectors.

#pragma once

#include "config.h"
#include "state.h"
#include <vector>
#include <algorithm>
#include <memory>
#include <iostream>

#ifdef _OPENMP
#include <omp.h>
#endif

namespace sorting_networks {

// ============================================================================
// Data Structures
// ============================================================================

struct Operation {
    byte op1 = 0;
    byte op2 = 0;
};

struct StateSuccessor {
    word beam_index = 0;
    byte op1 = 0;
    byte op2 = 0;
    double score = 0.0;
};

// ============================================================================
// Beam Storage - now dynamically sized
// ============================================================================

class BeamSearchContext {
public:
    std::vector<std::vector<Operation>> beam;
    std::vector<std::vector<Operation>> temp_beam;
    std::vector<StateSuccessor> beam_successors;
    int current_beam_size = 1;
    
    BeamSearchContext() {
        resize();
    }
    
    void resize() {
        const int max_beam = g_config.max_beam_size;
        const int max_ops = g_config.max_ops;
        const int branching = g_config.branching_factor;
        
        // Use assign to reinitialize all elements, not just add new ones
        beam.assign(max_beam, std::vector<Operation>(max_ops));
        temp_beam.assign(max_beam, std::vector<Operation>(max_ops));
        beam_successors.reserve(max_beam * branching);
    }
    
    [[nodiscard]] int beam_search(State& result);
};

// Global instance
inline BeamSearchContext g_beam_context;

// ============================================================================
// Beam Search Implementation
// ============================================================================

inline int BeamSearchContext::beam_search(State& result) {
    const int net_size = g_config.net_size;
    const int max_beam_size = g_config.max_beam_size;
    const int max_ops = g_config.max_ops;
    const bool use_asymmetry = g_config.use_asymmetry_heuristic;
    
    // Ensure beam storage is sized correctly
    if (static_cast<int>(beam.size()) != max_beam_size || 
        (beam.size() > 0 && static_cast<int>(beam[0].size()) != max_ops)) {
        resize();
    }
    
    current_beam_size = 1;
    State beam_state;
    
    for (int level = 0; ; ++level) {
        if (current_beam_size < max_beam_size)
            std::cout << level << " [" << current_beam_size << "] ";
        else
            std::cout << level << " ";
        std::cout.flush();
        
        beam_successors.clear();
        
        for (int i = 0; i < current_beam_size; ++i) {
            beam_state.set_start_state();
            for (int j = 0; j < level; ++j) {
                beam_state.update_state(beam[i][j].op1, beam[i][j].op2);
            }
            
            // Dynamic 2D vector for successor operations
            std::vector<std::vector<int>> succ_ops(net_size, std::vector<int>(net_size, 0));
            int num_succs = beam_state.find_successors(succ_ops);
            
            if (num_succs == 0) {
                std::cout << std::endl;
                result = beam_state;
                return result.current_level;
            }
            
            bool skip_search = false;
            if (use_asymmetry && level >= 1) {
                int n1 = beam[i][level - 1].op1;
                int n2 = beam[i][level - 1].op2;
                int inv_n1 = (net_size - 1) - n2;
                int inv_n2 = (net_size - 1) - n1;
                
                if (n1 != (net_size - 1) - n1 && n1 != (net_size - 1) - n2 &&
                    n2 != (net_size - 1) - n1 && n2 != (net_size - 1) - n2 &&
                    succ_ops[inv_n1][inv_n2] == 1) {
                    
                    State temp_state = beam_state;
                    temp_state.update_state(inv_n1, inv_n2);
                    
                    StateSuccessor succ;
                    succ.beam_index = static_cast<word>(i);
                    succ.op1 = static_cast<byte>(inv_n1);
                    succ.op2 = static_cast<byte>(inv_n2);
                    succ.score = temp_state.score_state();
                    beam_successors.push_back(succ);
                    skip_search = true;
                }
            }
            
            if (!skip_search) {
                #pragma omp parallel for
                for (int k = 0; k < net_size * (net_size - 1) / 2; ++k) {
                    int n1 = k / net_size;
                    int n2 = k % net_size;
                    if (n2 <= n1) {
                        n1 = net_size - n1 - 2;
                        n2 = net_size - n2 - 1;
                    }
                    
                    if (succ_ops[n1][n2] == 1) {
                        auto temp_state = std::make_unique<State>(beam_state);
                        temp_state->update_state(n1, n2);
                        double score = temp_state->score_state();
                        
                        #pragma omp critical
                        {
                            StateSuccessor succ;
                            succ.beam_index = static_cast<word>(i);
                            succ.op1 = static_cast<byte>(n1);
                            succ.op2 = static_cast<byte>(n2);
                            succ.score = score;
                            beam_successors.push_back(succ);
                        }
                    }
                }
            }
        }
        
        // Sort using std::sort
        std::sort(beam_successors.begin(), beam_successors.end(),
                  [](const auto& a, const auto& b) { return a.score < b.score; });
        
        current_beam_size = std::min(static_cast<int>(beam_successors.size()), max_beam_size);
        
        // Update beams
        for (int i = 0; i < current_beam_size; ++i) {
            for (int j = 0; j < level; ++j)
                temp_beam[i][j] = beam[beam_successors[i].beam_index][j];
            temp_beam[i][level].op1 = beam_successors[i].op1;
            temp_beam[i][level].op2 = beam_successors[i].op2;
        }
        
        for (int i = 0; i < current_beam_size; ++i)
            for (int j = 0; j <= level; ++j)
                beam[i][j] = temp_beam[i][j];
    }
}

} // namespace sorting_networks
