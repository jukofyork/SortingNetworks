// config.cpp
// ==========
// Command-line argument parsing implementation.

#include "config.h"
#include <iostream>
#include <cstdlib>

namespace sorting_networks {

void Config::initialize() {
    // Validate net_size
    if (net_size < 2 || net_size > 32) {
        throw std::invalid_argument("net_size must be between 2 and 32");
    }
    
    // Validate bounds exist for this net_size
    auto bounds = get_bounds(net_size);
    if (bounds.length == 0 || bounds.depth == 0) {
        throw std::invalid_argument("No known bounds for net_size " + std::to_string(net_size));
    }
    
    // Set default asymmetry heuristic: on for even, off for odd
    // (unless explicitly overridden by command line)
    if (!asymmetry_explicitly_set) {
        use_asymmetry_heuristic = (net_size % 2 == 0);
    }
    
    // Validate max_beam_size
    if (max_beam_size < 1) {
        throw std::invalid_argument("max_beam_size must be at least 1");
    }
    
    // Validate num_test_runs
    if (num_test_runs < 1) {
        throw std::invalid_argument("num_test_runs must be at least 1");
    }
    
    // Validate num_test_run_elites
    if (num_test_run_elites < 1) {
        throw std::invalid_argument("num_test_run_elites must be at least 1");
    }
    if (num_test_run_elites > num_test_runs) {
        throw std::invalid_argument("num_test_run_elites cannot exceed num_test_runs");
    }
    
    // Validate depth_weight
    if (depth_weight < 0.0 || depth_weight > 1.0) {
        throw std::invalid_argument("depth_weight must be between 0.0 and 1.0");
    }
    
    // Validate max_iterations
    if (max_iterations < 1) {
        throw std::invalid_argument("max_iterations must be at least 1");
    }
    
    branching_factor = (net_size * (net_size - 1)) / 2;
    num_tests = 1 << net_size;
    
    length_upper_bound = bounds.length;
    depth_upper_bound = bounds.depth;
    max_ops = length_upper_bound * 2;
}

void Config::print_usage(const char* program_name) {
    std::cout << "Usage: " << program_name << " [options]\n"
              << "\n"
              << "Options:\n"
              << "  -n, --net-size SIZE          Network size (2-32, default: 16)\n"
              << "  -b, --beam-size SIZE         Beam width (default: 1000)\n"
              << "  -t, --test-runs N            Number of test runs per score (default: 10)\n"
              << "  -e, --elites N               Number of elite samples to average (default: 1)\n"
              << "  -w, --depth-weight W         Weight for depth vs length optimization (0.0-1.0, default: 0.0001)\n"
              << "  -a, --asymmetry              Enable asymmetry heuristic\n"
              << "  -A, --no-asymmetry           Disable asymmetry heuristic\n"
              << "                               (default: on for even net_size, off for odd)\n"
              << "  -i, --max-iterations N       Maximum iterations (default: 1000000)\n"
              << "  -h, --help                   Show this help message\n"
              << "\n"
              << "Examples:\n"
              << "  " << program_name << " -n 8                    # Search for size-8 network\n"
              << "  " << program_name << " -n 12 -b 500 -t 5       # Search with smaller beam\n"
              << "  " << program_name << " -n 17 -a                # Force asymmetry for odd size\n"
              << "  " << program_name << " -n 16 -A                # Disable asymmetry for even size\n";
}

void Config::parse_args(int argc, char* argv[]) {
    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        
        if (arg == "-h" || arg == "--help") {
            print_usage(argv[0]);
            std::exit(0);
        }
        else if ((arg == "-n" || arg == "--net-size") && i + 1 < argc) {
            net_size = std::atoi(argv[++i]);
        }
        else if ((arg == "-b" || arg == "--beam-size") && i + 1 < argc) {
            max_beam_size = std::atoi(argv[++i]);
        }
        else if ((arg == "-t" || arg == "--test-runs") && i + 1 < argc) {
            num_test_runs = std::atoi(argv[++i]);
        }
        else if ((arg == "-e" || arg == "--elites") && i + 1 < argc) {
            num_test_run_elites = std::atoi(argv[++i]);
        }
        else if ((arg == "-w" || arg == "--depth-weight") && i + 1 < argc) {
            depth_weight = std::atof(argv[++i]);
        }
        else if (arg == "-a" || arg == "--asymmetry") {
            use_asymmetry_heuristic = true;
            asymmetry_explicitly_set = true;
        }
        else if (arg == "-A" || arg == "--no-asymmetry") {
            use_asymmetry_heuristic = false;
            asymmetry_explicitly_set = true;
        }
        else if ((arg == "-i" || arg == "--max-iterations") && i + 1 < argc) {
            max_iterations = std::atoi(argv[++i]);
        }
        else {
            std::cerr << "Unknown option: " << arg << "\n";
            print_usage(argv[0]);
            std::exit(1);
        }
    }
    
    initialize();
}

void Config::print() const {
    std::cout << "NET_SIZE                = " << net_size << "\n"
              << "MAX_BEAM_SIZE           = " << max_beam_size << "\n"
              << "NUM_TEST_RUNS           = " << num_test_runs << "\n"
              << "NUM_TEST_RUN_ELITES     = " << num_test_run_elites << "\n"
              << "USE_ASYMMETRY_HEURISTIC = " << (use_asymmetry_heuristic ? "Yes" : "No") << "\n"
              << "DEPTH_WEIGHT            = " << depth_weight << "\n"
              << "LENGTH_UPPER_BOUND      = " << length_upper_bound << "\n"
              << "DEPTH_UPPER_BOUND       = " << depth_upper_bound << "\n"
              << "MAX_OPS                 = " << max_ops << "\n"
              << "NUM_TESTS               = " << num_tests << "\n"
              << std::endl;
}

} // namespace sorting_networks
