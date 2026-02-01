// SortingNetworks.cpp
// ===================
// Modern C++20 beam search for finding minimum length sorting networks.

#include "config.h"
#include "lookup.h"
#include "state.h"
#include "search.h"

#include <iostream>
#include <ctime>
#include <csignal>
#include <sys/resource.h>
#include <atomic>
#include <memory>

namespace sorting_networks {

// Modern signal handling
std::atomic<bool> exit_flag{false};

void signal_handler(int) {
    if (exit_flag.load()) {
        std::exit(1);
    }
    exit_flag.store(true);
    std::signal(SIGINT, signal_handler);
}

void print_results(const State& state, int length, int depth) {
    for (int i = 0; i < state.current_level; ++i) {
        std::cout << '+' << (i + 1) << ":(" << static_cast<int>(state.operations[i][0]) << ','
                  << static_cast<int>(state.operations[i][1]) << ")" << std::endl;
    }
    std::cout << "+Length: " << length << std::endl;
    std::cout << "+Depth : " << depth << std::endl;
    std::cout << std::endl;
}

} // namespace sorting_networks

int main(int argc, char* argv[]) {
    using namespace sorting_networks;
    
    // Parse command-line arguments
    try {
        g_config.parse_args(argc, argv);
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << "\n";
        return 1;
    }
    
    // Use smart pointer for heap allocation
    auto state = std::make_unique<State>();
    
    std::clock_t start_time = std::clock();
    
    // Modern signal handling
    std::signal(SIGINT, signal_handler);
    
    // Set lowest priority
    setpriority(PRIO_PROCESS, 0, 19);
    
    // Initialize lookup tables
    g_lookups.initialize();
    
    // Resize beam context for the configured sizes
    g_beam_context.resize();
    
    g_config.print();
    
    int iteration;
    for (iteration = 0; iteration < g_config.max_iterations && !exit_flag.load(); ++iteration) {
        std::cout << "Iteration " << (iteration + 1) << ':' << std::endl;
        
        int length = g_beam_context.beam_search(*state);
        state->minimise_depth();
        int depth = state->get_depth();
        
        print_results(*state, length, depth);
        
        if (length < g_config.length_upper_bound || depth < g_config.depth_upper_bound) {
            ++iteration;
            break;
        }
        
        // Reset for next iteration
        state = std::make_unique<State>();
        g_beam_context = BeamSearchContext{};  // Reset beam
        g_beam_context.resize();  // Re-initialize with correct sizes
    }
    
    double elapsed = static_cast<double>(std::clock() - start_time) / CLOCKS_PER_SEC;
    
    std::cout << "Total Iterations  : " << iteration << std::endl;
    std::cout << "Total Time        : " << elapsed << " seconds" << std::endl;
    
    return 0;
}

