#include "config.h"
#include "lookup.h"
#include "state.h"
#include "search.h"
#include "normalization.h"

#include <iostream>
#include <chrono>
#include <csignal>
#include <atomic>
#include <memory>

std::atomic<bool> exit_flag{false};

void signal_handler(int) {
    if (exit_flag.load()) {
        std::exit(1);
    }
    exit_flag.store(true);
    std::signal(SIGINT, signal_handler);
}

template<int NetSize>
void print_results(const State<NetSize>& state, int length, int depth, int net_size) {
    // Make a copy and canonicalize the operations
    std::vector<Operation> normalized_ops;
    normalized_ops.reserve(state.current_level);
    for (int i = 0; i < state.current_level; ++i) {
        normalized_ops.push_back(state.operations[i]);
    }
    canonical_normalize<NetSize>(normalized_ops, state.current_level, net_size);

    // Print the canonicalized network
    for (int i = 0; i < state.current_level; ++i) {
        std::cout << '+' << (i + 1) << ":(" << static_cast<int>(normalized_ops[i].op1) << ','
                  << static_cast<int>(normalized_ops[i].op2) << ")" << std::endl;
    }
    std::cout << "+Length: " << length << std::endl;
    std::cout << "+Depth : " << depth << std::endl;
    std::cout << std::endl;
}

template<int NetSize>
void run_search(const Config& config) {
    LookupTables lookups;
    lookups.initialize(config);

    BeamSearchContext beam_context(config);

    auto state = std::make_unique<State<NetSize>>(config);

    auto start_time = std::chrono::steady_clock::now();

    std::signal(SIGINT, signal_handler);

    config.print();

    int current_iteration;
    for (current_iteration = 0; current_iteration < config.get_max_iterations() && !exit_flag.load(); ++current_iteration) {
        std::cout << "Iteration " << (current_iteration + 1) << ':' << std::endl;

        int length = beam_context.beam_search(*state, config, lookups);
        state->minimise_depth(config.get_net_size());
        int depth = state->get_depth(config.get_net_size());

        print_results(*state, length, depth, config.get_net_size());

        if (length < config.get_length_lower_bound() || depth < config.get_depth_lower_bound()) {
            ++current_iteration;
            break;
        }

        state = std::make_unique<State<NetSize>>(config);
        beam_context = BeamSearchContext(config);
    }

    auto end_time = std::chrono::steady_clock::now();
    auto elapsed = std::chrono::duration<double>(end_time - start_time).count();

    std::cout << "Total Iterations  : " << current_iteration << std::endl;
    std::cout << "Total Time        : " << elapsed << " seconds" << std::endl;
}

int main(int argc, char* argv[]) {
    Config config;

    try {
        config.parse_args(argc, argv);
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << "\n";
        return 1;
    }

    switch (config.get_net_size()) {
        case 2: run_search<2>(config); break;
        case 3: run_search<3>(config); break;
        case 4: run_search<4>(config); break;
        case 5: run_search<5>(config); break;
        case 6: run_search<6>(config); break;
        case 7: run_search<7>(config); break;
        case 8: run_search<8>(config); break;
        case 9: run_search<9>(config); break;
        case 10: run_search<10>(config); break;
        case 11: run_search<11>(config); break;
        case 12: run_search<12>(config); break;
        case 13: run_search<13>(config); break;
        case 14: run_search<14>(config); break;
        case 15: run_search<15>(config); break;
        case 16: run_search<16>(config); break;
        case 17: run_search<17>(config); break;
        case 18: run_search<18>(config); break;
        case 19: run_search<19>(config); break;
        case 20: run_search<20>(config); break;
        case 21: run_search<21>(config); break;
        case 22: run_search<22>(config); break;
        case 23: run_search<23>(config); break;
        case 24: run_search<24>(config); break;
        case 25: run_search<25>(config); break;
        case 26: run_search<26>(config); break;
        case 27: run_search<27>(config); break;
        case 28: run_search<28>(config); break;
        case 29: run_search<29>(config); break;
        case 30: run_search<30>(config); break;
        case 31: run_search<31>(config); break;
        case 32: run_search<32>(config); break;
        default:
            std::cerr << "Error: Unsupported net_size. Must be between 2 and 32.\n";
            return 1;
    }

    return 0;
}
