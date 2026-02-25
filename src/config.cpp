#include "config.h"
#include <iostream>
#include <cstdlib>
#include <stdexcept>

void Config::initialize() {
    if (net_size_ < 2 || net_size_ > 32) {
        throw std::invalid_argument("net_size must be between 2 and 32");
    }

    auto bounds = get_bounds(net_size_);
    if (bounds.length == 0 || bounds.depth == 0) {
        throw std::invalid_argument("No known bounds for net_size " + std::to_string(net_size_));
    }

    if (!symmetry_explicitly_set_) {
        use_symmetry_heuristic_ = (net_size_ % 2 == 0);
    }

    if (max_beam_size_ < 1) {
        throw std::invalid_argument("max_beam_size must be at least 1");
    }

    if (num_scoring_iterations_ < 1) {
        throw std::invalid_argument("num_scoring_iterations must be at least 1");
    }

    if (num_elites_ < 1) {
        throw std::invalid_argument("num_elites must be at least 1");
    }
    if (num_elites_ > num_scoring_iterations_) {
        throw std::invalid_argument("num_elites cannot exceed num_scoring_iterations");
    }

    if (depth_weight_ < 0.0 || depth_weight_ > 1.0) {
        throw std::invalid_argument("depth_weight must be between 0.0 and 1.0");
    }

    if (max_iterations_ < 1) {
        throw std::invalid_argument("max_iterations must be at least 1");
    }

    branching_factor_ = (net_size_ * (net_size_ - 1)) / 2;
    num_input_patterns_ = static_cast<std::size_t>(1ULL) << net_size_;

    if (net_size_ <= 8) {
        input_pattern_type_ = "uint8_t";
    } else if (net_size_ <= 16) {
        input_pattern_type_ = "uint16_t";
    } else {
        input_pattern_type_ = "uint32_t";
    }

    length_lower_bound_ = bounds.length;
    length_upper_bound_ = length_lower_bound_ * 2;
    depth_lower_bound_ = bounds.depth;
}

void Config::print_usage(const char* program_name) const {
    std::cout << "Usage: " << program_name << " [options]\n"
              << "\n"
              << "Options:\n"
              << "  -i, --max-iterations N       Maximum search iterations (default: " << max_iterations_ << ")\n"
              << "  -n, --net-size SIZE          Network size, 2-32 (default: " << net_size_ << ")\n"
              << "                               Note: Sizes > 20 require significant memory (2^n patterns)\n"
              << "  -b, --beam-size SIZE         Beam width (default: " << max_beam_size_ << ")\n"
              << "  -t, --scoring-tests N        Number of scoring tests per state (default: " << num_scoring_iterations_ << ")\n"
              << "  -e, --elite-tests N          Number of elite tests to average (default: " << num_elites_ << ")\n"
              << "  -s, --symmetry               Enable symmetry heuristic\n"
              << "  -S, --no-symmetry            Disable symmetry heuristic\n"
              << "                               (default: on for even net_size, off for odd)\n"
              << "  -w, --depth-weight W         Weight for depth vs length, 0.0-1.0 (default: " << depth_weight_ << ")\n"
              << "  -h, --help                   Show this help message\n"
              << "\n"
              << "Examples:\n"
              << "  " << program_name << " -n 8                    # Search for size-8 network\n"
              << "  " << program_name << " -n 12 -b 500 -t 5       # Search with larger beam\n"
              << "  " << program_name << " -n 17 -s                # Force symmetry for odd size\n"
              << "  " << program_name << " -n 16 -S                # Disable symmetry for even size\n";
}

void Config::parse_args(int argc, char* argv[]) {
    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];

        if (arg == "-h" || arg == "--help") {
            print_usage(argv[0]);
            std::exit(0);
        }
        else if ((arg == "-n" || arg == "--net-size") && i + 1 < argc) {
            try {
                net_size_ = std::stoi(argv[++i]);
            } catch (const std::exception&) {
                throw std::invalid_argument("Invalid value for --net-size");
            }
        }
        else if ((arg == "-b" || arg == "--beam-size") && i + 1 < argc) {
            try {
                max_beam_size_ = std::stoi(argv[++i]);
            } catch (const std::exception&) {
                throw std::invalid_argument("Invalid value for --beam-size");
            }
        }
        else if ((arg == "-t" || arg == "--scoring-iterations") && i + 1 < argc) {
            try {
                num_scoring_iterations_ = std::stoi(argv[++i]);
            } catch (const std::exception&) {
                throw std::invalid_argument("Invalid value for --scoring-iterations");
            }
        }
        else if ((arg == "-e" || arg == "--elites") && i + 1 < argc) {
            try {
                num_elites_ = std::stoi(argv[++i]);
            } catch (const std::exception&) {
                throw std::invalid_argument("Invalid value for --elites");
            }
        }
        else if ((arg == "-w" || arg == "--depth-weight") && i + 1 < argc) {
            try {
                depth_weight_ = std::stod(argv[++i]);
            } catch (const std::exception&) {
                throw std::invalid_argument("Invalid value for --depth-weight");
            }
        }
        else if (arg == "-s" || arg == "--symmetry") {
            use_symmetry_heuristic_ = true;
            symmetry_explicitly_set_ = true;
        }
        else if (arg == "-S" || arg == "--no-symmetry") {
            use_symmetry_heuristic_ = false;
            symmetry_explicitly_set_ = true;
        }
        else if ((arg == "-i" || arg == "--max-iterations") && i + 1 < argc) {
            try {
                max_iterations_ = std::stoi(argv[++i]);
            } catch (const std::exception&) {
                throw std::invalid_argument("Invalid value for --max-iterations");
            }
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
    std::cout << "MAX_ITERATIONS          = " << max_iterations_ << "\n"
              << "NET_SIZE                = " << net_size_ << "\n"
              << "MAX_BEAM_SIZE           = " << max_beam_size_ << "\n"
              << "NUM_SCORING_TESTS       = " << num_scoring_iterations_ << "\n"
              << "NUM_ELITE_TESTS         = " << num_elites_ << "\n"
              << "USE_SYMMETRY_HEURISTIC  = " << (use_symmetry_heuristic_ ? "Yes" : "No") << "\n"
              << "DEPTH_WEIGHT            = " << depth_weight_ << "\n"
              << "NUM_INPUT_PATTERNS      = " << num_input_patterns_ << "\n"
              << "INPUT_PATTERN_TYPE      = " << input_pattern_type_ << "\n"
              << "LENGTH_LOWER_BOUND      = " << length_lower_bound_ << "\n"
              << "LENGTH_UPPER_BOUND      = " << length_upper_bound_ << "\n"
              << "DEPTH_LOWER_BOUND       = " << depth_lower_bound_ << "\n"
              << std::endl;
}
