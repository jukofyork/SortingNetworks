# AGENTS.md - SortingNetworks

Guidelines for AI agents working on this C++20 beam search implementation for finding optimal sorting networks.

## Build Commands

```bash
# Release build (optimized)
make
make release

# Debug build (with symbols, no optimization)
make debug

# Clean build artifacts
make clean

# Build and run
make run

# Run tests
./test.sh
```

## Project Structure

- **Language**: C++20 with GCC
- **Build System**: Makefile
- **Parallelism**: OpenMP
- **No namespaces**: Everything is in global scope

## Code Style Guidelines

### Naming Conventions
- **Types**: PascalCase (`Config`, `State`, `BeamSearchContext`)
- **Functions/Methods**: snake_case (`minimise_depth()`, `get_depth()`)
- **Member Variables**: snake_case (`current_level`, `max_beam_size`)
- **No global variables**: Pass configuration and lookup tables by reference
- **Type Aliases**: Use standard types (`std::uint8_t`, `std::uint16_t`, `std::size_t`)
- **Constants**: ALL_CAPS or snake_case (`MAX_BEAM_SIZE`, `net_size`)
- **Template Parameters**: PascalCase

### File Organization
- One header/implementation pair per logical component
- Headers use `#pragma once`
- No file comment banners or section dividers

### Formatting
- **Indentation**: 4 spaces (no tabs)
- **Line Length**: ~100 characters
- **Braces**:
  - Classes: Opening brace on new line
  - Functions/methods: Opening brace on same line
  - Control structures: Opening brace on same line
- **Spacing**: Space after control keywords (`if (`, `for (`)

### Includes
```cpp
// 1. Own header first
#include "config.h"

// 2. Project headers
#include "lookup.h"
#include "search.h"
#include "state.h"

// 3. Standard library headers (alphabetical)
#include <algorithm>
#include <array>
#include <atomic>
#include <iostream>
#include <memory>
#include <vector>
```

### Modern C++ Features
- Use `std::make_unique()` for heap allocation
- Mark functions `[[nodiscard]]` when return value shouldn't be ignored
- Use digit separators: `1'000'000`
- Prefer `std::vector` over raw arrays
- Use `std::array` for fixed-size collections
- Thread-local storage: `thread_local std::mt19937`
- Use `std::thread` and `std::mutex` for parallelism

### Error Handling
- Use exceptions for configuration validation (`std::invalid_argument`)
- Use `std::exit()` only in signal handlers
- Always validate bounds before operations

### Classes
```cpp
class ClassName {
public:
    ClassName(const Config& config);

    void method(const LookupTables& lookups);

private:
    int member_ = 0;
};
```

### Config Class Ordering
Parameters in `Config` class follow consistent ordering:
1. User-configurable parameters (matching CLI option order)
2. Computed parameters (matching output print order)

This ensures getters, setters, print_usage(), print(), and initialize() are all consistent.

### Comments
- No file headers
- No section dividers
- Minimal inline comments for complex logic only
- Prefer `//` over `/* */`

### Dependencies
- Configuration and lookup tables are passed by const reference
- No global state
- Thread-safe using standard C++ synchronization primitives

### Performance
- Pass by const reference for large objects
- Use `reserve()` for vectors when size is known
- Mark functions inline when defined in headers
- Use `std::sort` with custom comparators

## Compiler Flags

Release: `-std=c++20 -O3 -march=native -Wall -Wextra -Wpedantic -fopenmp`
Debug: `-std=c++20 -g -O0 -Wall -Wextra -Wpedantic -fopenmp`

## Testing

Run the test script:
```bash
./test.sh
```

This will:
1. Build the project
2. Test command-line options
3. Run the algorithm for 10-15 seconds
4. Verify output format

## Key Patterns

- Dependencies passed by reference: `const Config& config`, `const LookupTables& lookups`
- Smart pointers for RAII: `std::make_unique<State>(config)`
- Bit manipulation for vector representation
- Beam search with parallel successor evaluation using `std::thread`
- Stochastic scoring with thread-local RNG
