#include "state.h"
#include "search.h"
#include <chrono>
#include <iostream>
#include <string>

const int iterations = 10'000;

template<int NetSize>
void benchmark_score_state(const Config& config, const LookupTables& lookups) {
    State<NetSize> state(config);
    state.set_start_state(config, lookups);

    // Warmup
    for (int i = 0; i < 100; ++i) {
        static_cast<void>(state.score_state(5, 0.0001, lookups));
    }

    auto start = std::chrono::steady_clock::now();
    for (int i = 0; i < iterations; ++i) {
        static_cast<void>(state.score_state(5, 0.0001, lookups));
    }
    auto end = std::chrono::steady_clock::now();

    double elapsed = std::chrono::duration<double>(end - start).count();
    std::cout << NetSize << ": " << (iterations / elapsed) << " calls/sec\n";
}

template<int NetSize>
void benchmark_update_state(const Config& config, const LookupTables& lookups) {
    State<NetSize> state(config);
    state.set_start_state(config, lookups);

    // Warmup
    for (int i = 0; i < 100; ++i) {
        state.update_state(0, 1, lookups);
        state.set_start_state(config, lookups);
    }

    auto start = std::chrono::steady_clock::now();
    for (int i = 0; i < iterations; ++i) {
        state.update_state(0, 1, lookups);
        state.set_start_state(config, lookups);
    }
    auto end = std::chrono::steady_clock::now();

    double elapsed = std::chrono::duration<double>(end - start).count();
    std::cout << NetSize << ": " << (iterations / elapsed) << " calls/sec\n";
}

template<int NetSize>
void benchmark_do_random_transition(const Config& config, const LookupTables& lookups) {
    State<NetSize> state(config);
    state.set_start_state(config, lookups);

    // Warmup
    for (int i = 0; i < 100; ++i) {
        if (state.num_unsorted > 0) {
            state.do_random_transition(lookups);
        } else {
            state.set_start_state(config, lookups);
        }
    }

    state.set_start_state(config, lookups);
    auto start = std::chrono::steady_clock::now();
    for (int i = 0; i < iterations; ++i) {
        if (state.num_unsorted > 0) {
            state.do_random_transition(lookups);
        } else {
            state.set_start_state(config, lookups);
        }
    }
    auto end = std::chrono::steady_clock::now();

    double elapsed = std::chrono::duration<double>(end - start).count();
    std::cout << NetSize << ": " << (iterations / elapsed) << " calls/sec\n";
}

template<int NetSize>
void run_benchmarks_for_size() {
    Config config;
    std::string size_str = std::to_string(NetSize);
    char size_buf[16];
    std::copy(size_str.begin(), size_str.end(), size_buf);
    size_buf[size_str.size()] = '\0';

    char* argv[] = {const_cast<char*>("benchmark"), const_cast<char*>("-n"), size_buf};
    int argc = 3;
    config.parse_args(argc, argv);

    LookupTables lookups;
    lookups.initialize(config);

    std::cout << "Benchmarking score_state for NetSize=" << NetSize << "...\n";
    benchmark_score_state<NetSize>(config, lookups);

    std::cout << "Benchmarking update_state for NetSize=" << NetSize << "...\n";
    benchmark_update_state<NetSize>(config, lookups);

    std::cout << "Benchmarking do_random_transition for NetSize=" << NetSize << "...\n";
    benchmark_do_random_transition<NetSize>(config, lookups);

    std::cout << "\n";
}

int main() {
    std::cout << "=== Sorting Network Performance Benchmarks ===\n\n";

    run_benchmarks_for_size<8>();
    run_benchmarks_for_size<10>();
    run_benchmarks_for_size<12>();

    std::cout << "Benchmarks completed.\n";
    return 0;
}
