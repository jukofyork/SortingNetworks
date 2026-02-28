#pragma once

#include "types.h"
#include <vector>
#include <array>
#include <algorithm>
#include <cstdint>

// Canonical normalization for sorting networks.
// Maps isomorphic networks to the same canonical form for deduplication.
// Based on Choi & Moon's paper "Isomorphism, Normalization, and a Genetic Algorithm
// for Sorting Network Optimization".

namespace {

// FNV-1a hash constants
constexpr std::uint64_t FNV_OFFSET = 14695981039346656037ULL;
constexpr std::uint64_t FNV_PRIME = 1099511628211ULL;

// Compute FNV-1a hash of operation sequence
inline std::uint64_t fnv1a_hash(const std::vector<Operation>& ops, int num_ops) {
    std::uint64_t hash = FNV_OFFSET;
    for (int i = 0; i < num_ops; ++i) {
        hash ^= static_cast<std::uint64_t>(ops[i].op1);
        hash *= FNV_PRIME;
        hash ^= static_cast<std::uint64_t>(ops[i].op2);
        hash *= FNV_PRIME;
    }
    return hash;
}

} // anonymous namespace

// Compute degree of each bus based on operation sequence
template<int NetSize>
void compute_bus_degrees(const std::vector<Operation>& ops, int num_ops, 
                         std::array<int, MAX_NET_SIZE>& degrees) {
    degrees.fill(0);
    for (int i = 0; i < num_ops; ++i) {
        degrees[ops[i].op1]++;
        degrees[ops[i].op2]++;
    }
}

// Compute sum of neighbor degrees for tie-breaking
template<int NetSize>
void compute_neighbor_sums(const std::vector<Operation>& ops, int num_ops,
                           const std::array<int, MAX_NET_SIZE>& degrees,
                           std::array<int, MAX_NET_SIZE>& neighbor_sums) {
    neighbor_sums.fill(0);
    for (int i = 0; i < num_ops; ++i) {
        neighbor_sums[ops[i].op1] += degrees[ops[i].op2];
        neighbor_sums[ops[i].op2] += degrees[ops[i].op1];
    }
}

// Greedy canonical labeling algorithm from Choi & Moon paper
// Assigns labels based on structural properties (degree, connectivity)
template<int NetSize>
std::array<std::uint8_t, 32> compute_canonical_mapping(const std::vector<Operation>& ops, 
                                                       int num_ops, int net_size) {
    std::array<std::uint8_t, MAX_NET_SIZE> mapping;
    std::array<bool, MAX_NET_SIZE> assigned;
    mapping.fill(INVALID_LABEL);  // Invalid marker
    assigned.fill(false);
    
    // Compute structural signatures
    std::array<int, MAX_NET_SIZE> degrees;
    std::array<int, MAX_NET_SIZE> neighbor_sums;
    compute_bus_degrees<NetSize>(ops, num_ops, degrees);
    compute_neighbor_sums<NetSize>(ops, num_ops, degrees, neighbor_sums);
    
    // Greedily assign canonical labels
    for (int new_label = 0; new_label < net_size; ++new_label) {
        // Find unassigned bus with highest priority:
        // 1. Highest degree
        // 2. Tie-break: highest neighbor degree sum
        // 3. Tie-break: lowest original label (deterministic)
        int best_bus = -1;
        int best_degree = -1;
        int best_neighbor_sum = -1;
        
        for (int bus = 0; bus < net_size; ++bus) {
            if (assigned[bus]) continue;
            
            if (degrees[bus] > best_degree ||
                (degrees[bus] == best_degree && neighbor_sums[bus] > best_neighbor_sum) ||
                (degrees[bus] == best_degree && neighbor_sums[bus] == best_neighbor_sum && 
                 (best_bus == -1 || bus < best_bus))) {
                best_bus = bus;
                best_degree = degrees[bus];
                best_neighbor_sum = neighbor_sums[bus];
            }
        }
        
        if (best_bus >= 0) {
            mapping[best_bus] = static_cast<std::uint8_t>(new_label);
            assigned[best_bus] = true;
            
            // Update neighbor sums for remaining unassigned buses
            for (int i = 0; i < num_ops; ++i) {
                if (ops[i].op1 == best_bus && !assigned[ops[i].op2]) {
                    neighbor_sums[ops[i].op2] -= best_degree;
                } else if (ops[i].op2 == best_bus && !assigned[ops[i].op1]) {
                    neighbor_sums[ops[i].op1] -= best_degree;
                }
            }
        }
    }
    
    return mapping;
}

// Apply canonical mapping to a sequence of operations
template<int NetSize>
void apply_canonical_mapping(std::vector<Operation>& ops, int num_ops, 
                             const std::array<std::uint8_t, 32>& mapping) {
    for (int i = 0; i < num_ops; ++i) {
        std::uint8_t new_op1 = mapping[ops[i].op1];
        std::uint8_t new_op2 = mapping[ops[i].op2];
        // Ensure op1 < op2 after relabeling
        if (new_op1 > new_op2) {
            std::swap(new_op1, new_op2);
        }
        ops[i].op1 = new_op1;
        ops[i].op2 = new_op2;
    }
}

// Sort operations within each parallel layer for consistent representation
template<int NetSize>
void normalize_operation_order(std::vector<Operation>& ops, int num_ops, int net_size) {
    // Group operations into parallel layers
    std::vector<std::vector<Operation>> layers;
    std::vector<bool> used(net_size, false);
    
    for (int i = 0; i < num_ops; ) {
        std::vector<Operation> layer;
        std::fill(used.begin(), used.end(), false);
        
        // Collect all operations that can run in parallel starting from position i
        for (int j = i; j < num_ops; ++j) {
            if (!used[ops[j].op1] && !used[ops[j].op2]) {
                layer.push_back(ops[j]);
                used[ops[j].op1] = true;
                used[ops[j].op2] = true;
            }
        }
        
        if (layer.empty()) {
            // Should not happen if operations are valid, but handle gracefully
            layer.push_back(ops[i]);
            i++;
        } else {
            i += static_cast<int>(layer.size());
        }
        
        // Sort operations within layer by first operand
        std::sort(layer.begin(), layer.end(), 
                  [](const Operation& a, const Operation& b) {
                      return a.op1 < b.op1 || (a.op1 == b.op1 && a.op2 < b.op2);
                  });
        
        layers.push_back(std::move(layer));
    }
    
    // Flatten back to operation sequence
    int idx = 0;
    for (const auto& layer : layers) {
        for (const auto& op : layer) {
            ops[idx++] = op;
        }
    }
}

// Full canonical normalization: compute mapping, apply it, and normalize order
template<int NetSize>
void canonical_normalize(std::vector<Operation>& ops, int num_ops, int net_size) {
    if (num_ops <= 0) return;
    
    // Step 1: Compute and apply canonical mapping
    auto mapping = compute_canonical_mapping<NetSize>(ops, num_ops, net_size);
    apply_canonical_mapping<NetSize>(ops, num_ops, mapping);
    
    // Step 2: Normalize order within parallel layers
    normalize_operation_order<NetSize>(ops, num_ops, net_size);
}

// Compute canonical hash of operation sequence
// This ensures isomorphic networks produce identical hashes
template<int NetSize>
std::uint64_t compute_canonical_hash(const std::vector<Operation>& ops, int num_ops, int net_size) {
    if (num_ops <= 0) return 0;
    
    // Make a copy since canonical_normalize modifies in place
    thread_local std::vector<Operation> normalized_ops;
    normalized_ops.clear();
    normalized_ops.reserve(num_ops);
    for (int i = 0; i < num_ops; ++i) {
        normalized_ops.push_back(ops[i]);
    }
    
    // Apply canonical normalization
    canonical_normalize<NetSize>(normalized_ops, num_ops, net_size);
    
    // Compute hash on normalized sequence
    return fnv1a_hash(normalized_ops, num_ops);
}
