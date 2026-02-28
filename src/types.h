#pragma once

#include <cstdint>
#include <type_traits>

struct Operation {
    std::uint8_t op1 = 0;
    std::uint8_t op2 = 0;
};

struct StateSuccessor {
    std::size_t beam_index = 0;
    Operation operation;
    double score = 0.0;
};

template<int N>
struct BitStorage {
    using type = std::conditional_t<(N <= 8), std::uint8_t,
                   std::conditional_t<(N <= 16), std::uint16_t,
                   std::uint32_t>>;
};

inline constexpr int END_OF_LIST = -1;
inline constexpr std::uint8_t INVALID_LABEL = 255;
inline constexpr int MAX_NET_SIZE = 32;
inline constexpr int NUM_NET_SIZE_CASES = 31;
