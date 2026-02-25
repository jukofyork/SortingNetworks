#pragma once

#include "config.h"
#include "state.h"
#include "types.h"
#include "normalization.h"
#include <vector>
#include <algorithm>
#include <memory>
#include <iostream>
#include <cstdint>
#include <chrono>
#include <unordered_set>
#include <unordered_map>

// Profiling macros for timing beam search phases.
// Define ENABLE_PROFILING before including this header to enable timing output.
#ifdef ENABLE_PROFILING
    #define PROFILE_START(name) auto name##_start = std::chrono::steady_clock::now()
    #define PROFILE_END(name, desc) \
        do { \
            auto name##_end = std::chrono::steady_clock::now(); \
            double name##_elapsed = std::chrono::duration<double>(name##_end - name##_start).count(); \
            std::cerr << "[PROFILE] " << desc << ": " << name##_elapsed << "s" << std::endl; \
        } while(0)
#else
    #define PROFILE_START(name)
    #define PROFILE_END(name, desc)
#endif

// Represents a candidate successor operation to be scored.
// Used to batch all scoring work into a single parallel region.
struct CandidateSuccessor {
    std::size_t beam_index;   // Which beam entry this belongs to
    std::uint8_t op1;         // First wire of comparator
    std::uint8_t op2;         // Second wire of comparator
    std::uint64_t canonical_hash; // Canonical hash for isomorphic deduplication
};

// BeamSearchContext maintains the state for beam search across iterations.
// The beam holds the top-k most promising partial networks found so far.
class BeamSearchContext {
public:
    // Each beam entry stores a sequence of operations (comparators).
    // beam[i][j] is the j-th operation of the i-th beam entry.
    std::vector<std::vector<Operation>> beam;

    // Temporary storage for building the next beam level.
    std::vector<std::vector<Operation>> temp_beam;

    // All candidate successors from current beam entries, scored and sorted.
    std::vector<StateSuccessor> beam_successors;

    // Buffer for collecting all candidate successors before parallel scoring.
    std::vector<CandidateSuccessor> candidates;

    int current_beam_size = 1;

    explicit BeamSearchContext(const Config& config) {
        resize(config);
    }

    void resize(const Config& config);

    // Perform beam search starting from an empty network.
    // Returns the length of the best network found.
    template<int NetSize>
    [[nodiscard]] int beam_search(State<NetSize>& result, const Config& config, const LookupTables& lookups);
};

inline void BeamSearchContext::resize(const Config& config) {
    const int max_beam_size = config.get_max_beam_size();
    const int max_ops = config.get_length_upper_bound();

    beam.assign(max_beam_size, std::vector<Operation>(max_ops));
    temp_beam.assign(max_beam_size, std::vector<Operation>(max_ops));
    beam_successors.reserve(static_cast<std::size_t>(max_beam_size) * config.get_branching_factor());
    candidates.reserve(static_cast<std::size_t>(max_beam_size) * config.get_branching_factor());
}

// Beam search algorithm for finding optimal sorting networks.
//
// The algorithm works level by level:
// 1. For each network in the current beam, find all valid next operations
// 2. Score each candidate network using Monte Carlo simulation
// 3. Keep the best candidates (up to beam_size) for the next level
// 4. Repeat until a complete sorting network is found
//
// Uses parallel candidate collection and scoring with OpenMP.
template<int NetSize>
int BeamSearchContext::beam_search(State<NetSize>& result, const Config& config, const LookupTables& lookups) {
    const int net_size = config.get_net_size();
    const int max_beam_size = config.get_max_beam_size();
    const int max_ops = config.get_length_upper_bound();
    const bool use_symmetry = config.get_use_symmetry_heuristic();

    if (static_cast<int>(beam.size()) != max_beam_size ||
        (beam.size() > 0 && static_cast<int>(beam[0].size()) != max_ops)) {
        resize(config);
    }

    current_beam_size = 1;

    for (int level = 0; ; ++level) {
        std::cout << level;
        std::cout.flush();

        beam_successors.clear();
        candidates.clear();

        PROFILE_START(successor_gen);
        PROFILE_START(candidate_collection);

        // Check for completed network and collect candidates in parallel
        int completed_index = -1;

        #pragma omp parallel
        {
            // Thread-local storage for candidates to avoid synchronization
            thread_local std::vector<CandidateSuccessor> local_candidates;
            local_candidates.clear();
            local_candidates.reserve(256);

            // Thread-local state for reconstruction
            thread_local State<NetSize> thread_state(config);
            thread_local std::vector<std::vector<int>> thread_succ_ops;
            thread_local std::vector<Operation> thread_ops;
            if (thread_succ_ops.empty()) {
                thread_succ_ops.reserve(net_size);
                for (int i = 0; i < net_size; ++i) {
                    thread_succ_ops.emplace_back(net_size, 0);
                }
            }

            #pragma omp for schedule(dynamic)
            for (int i = 0; i < current_beam_size; ++i) {
                // Check if another thread already found a complete network
                if (completed_index != -1) continue;

                // Reconstruct state for this beam entry
                thread_state.set_start_state(config, lookups);
                for (int j = 0; j < level; ++j) {
                    thread_state.update_state(beam[i][j].op1, beam[i][j].op2, lookups);
                }

                // Clear successor matrix
                for (auto& row : thread_succ_ops) {
                    std::fill(row.begin(), row.end(), 0);
                }
                int num_succs = thread_state.find_successors(thread_succ_ops, net_size);

                // Check for complete network
                if (num_succs == 0) {
                    #pragma omp critical
                    {
                        if (completed_index == -1) {
                            completed_index = i;
                        }
                    }
                    continue;
                }

                // Symmetry heuristic
                bool skip_search = false;
                if (use_symmetry && level >= 1) {
                    int n1 = beam[i][level - 1].op1;
                    int n2 = beam[i][level - 1].op2;
                    int inv_n1 = (net_size - 1) - n2;
                    int inv_n2 = (net_size - 1) - n1;

                    if (n1 != (net_size - 1) - n1 && n1 != (net_size - 1) - n2 &&
                        n2 != (net_size - 1) - n1 && n2 != (net_size - 1) - n2 &&
                        thread_succ_ops[inv_n1][inv_n2] == 1) {
                        // Build operation sequence and compute canonical hash
                        thread_ops.clear();
                        for (int j = 0; j < level; ++j) {
                            thread_ops.push_back(beam[i][j]);
                        }
                        thread_ops.push_back(Operation{static_cast<std::uint8_t>(inv_n1),
                                                       static_cast<std::uint8_t>(inv_n2)});
                        std::uint64_t hash = compute_canonical_hash<NetSize>(thread_ops,
                                                            level + 1, net_size);
                        local_candidates.push_back(CandidateSuccessor{static_cast<std::size_t>(i),
                                                                     static_cast<std::uint8_t>(inv_n1),
                                                                     static_cast<std::uint8_t>(inv_n2),
                                                                     hash});
                        skip_search = true;
                    }
                }

                // Collect valid successors
                if (!skip_search) {
                    for (int n1 = 0; n1 < net_size - 1; ++n1) {
                        for (int n2 = n1 + 1; n2 < net_size; ++n2) {
                            if (thread_succ_ops[n1][n2] == 1) {
                                // Build operation sequence and compute canonical hash
                                thread_ops.clear();
                                for (int j = 0; j < level; ++j) {
                                    thread_ops.push_back(beam[i][j]);
                                }
                                thread_ops.push_back(Operation{static_cast<std::uint8_t>(n1),
                                                               static_cast<std::uint8_t>(n2)});
                                std::uint64_t hash = compute_canonical_hash<NetSize>(thread_ops,
                                                                    level + 1, net_size);
                                local_candidates.push_back(CandidateSuccessor{static_cast<std::size_t>(i),
                                                                             static_cast<std::uint8_t>(n1),
                                                                             static_cast<std::uint8_t>(n2),
                                                                             hash});
                            }
                        }
                    }
                }
            }

            // Merge thread-local candidates into global list
            #pragma omp critical
            {
                candidates.insert(candidates.end(), local_candidates.begin(), local_candidates.end());
            }
        }

        PROFILE_END(candidate_collection, "Candidate collection (parallel)");

        // Deduplicate candidates using canonical hashing
        std::size_t before = candidates.size();
        if (!candidates.empty()) {
            std::unordered_map<std::uint64_t, std::size_t> hash_to_index;
            hash_to_index.reserve(candidates.size() * 2);

            std::size_t unique = 0;
            for (std::size_t i = 0; i < candidates.size(); ++i) {
                auto [it, inserted] = hash_to_index.try_emplace(candidates[i].canonical_hash, i);
                if (inserted) {
                    if (unique != i) {
                        candidates[unique] = candidates[i];
                    }
                    unique++;
                }
            }
            candidates.resize(unique);
        }
        std::size_t after = candidates.size();

        // Handle completed network found during collection
        if (completed_index != -1) {
            std::cout << std::endl;
            result.set_start_state(config, lookups);
            for (int j = 0; j < level; ++j) {
                result.update_state(beam[completed_index][j].op1, beam[completed_index][j].op2, lookups);
            }
            return level;
        }

        // Print reduction stats
        if (before == after) {
            std::cout << " [" << after << "], ";
        } else {
            std::cout << " [" << before << "\u2192" << after << "], ";
        }

        // Phase 2: Score all unique candidates in parallel
        beam_successors.resize(candidates.size());

        #pragma omp parallel
        {
            // Thread-local state for scoring
            thread_local State<NetSize> thread_state(config);

            #pragma omp for schedule(dynamic)
            for (std::size_t idx = 0; idx < candidates.size(); ++idx) {
                const auto& cand = candidates[idx];

                thread_state.set_start_state(config, lookups);
                for (int j = 0; j < level; ++j) {
                    thread_state.update_state(beam[cand.beam_index][j].op1,
                                              beam[cand.beam_index][j].op2, lookups);
                }

                thread_state.update_state(cand.op1, cand.op2, lookups);
                double score = thread_state.score_state(config, lookups);

                beam_successors[idx].beam_index = cand.beam_index;
                beam_successors[idx].operation.op1 = cand.op1;
                beam_successors[idx].operation.op2 = cand.op2;
                beam_successors[idx].score = score;
            }
        }

        PROFILE_END(successor_gen, "Successor generation (incl parallel scoring)");
        PROFILE_START(sorting);

        std::sort(beam_successors.begin(), beam_successors.end(),
                  [](const auto& a, const auto& b) { return a.score < b.score; });

        PROFILE_END(sorting, "Sorting successors");
        PROFILE_START(reconstruction);

        current_beam_size = std::min(static_cast<int>(beam_successors.size()), max_beam_size);

        for (int i = 0; i < current_beam_size; ++i) {
            for (int j = 0; j < level; ++j)
                temp_beam[i][j] = beam[beam_successors[i].beam_index][j];
            temp_beam[i][level] = beam_successors[i].operation;
        }

        for (int i = 0; i < current_beam_size; ++i)
            for (int j = 0; j <= level; ++j)
                beam[i][j] = temp_beam[i][j];

        PROFILE_END(reconstruction, "Beam reconstruction");
    }
}
