CXX := g++
CXXFLAGS := -std=c++20 -O3 -march=native -Wall -Wextra -Wpedantic -fopenmp
LDFLAGS := -fopenmp

TARGET := sorting_networks
BENCHMARK_TARGET := benchmark
SRCDIR := src

# Main program sources (exclude benchmark.cpp)
MAIN_SOURCES := $(SRCDIR)/SortingNetworks.cpp $(SRCDIR)/config.cpp
MAIN_OBJECTS := $(MAIN_SOURCES:.cpp=.o)

# Benchmark sources (exclude SortingNetworks.cpp which has its own main)
BENCHMARK_SOURCES := $(SRCDIR)/benchmark.cpp $(SRCDIR)/config.cpp
BENCHMARK_OBJECTS := $(BENCHMARK_SOURCES:.cpp=.o)

.PHONY: all clean release debug profile run bench

all: release

release: CXXFLAGS += -DNDEBUG
release: $(TARGET)

debug: CXXFLAGS := -std=c++20 -g -O0 -Wall -Wextra -Wpedantic -fopenmp
debug: $(TARGET)

profile: CXXFLAGS += -DENABLE_PROFILING
profile: $(TARGET)

bench: CXXFLAGS += -DNDEBUG
bench: $(BENCHMARK_TARGET)

$(TARGET): $(MAIN_OBJECTS)
	$(CXX) $(MAIN_OBJECTS) -o $@ $(LDFLAGS)

$(BENCHMARK_TARGET): $(BENCHMARK_OBJECTS)
	$(CXX) $(BENCHMARK_OBJECTS) -o $@ $(LDFLAGS)

$(SRCDIR)/%.o: $(SRCDIR)/%.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

clean:
	rm -f $(SRCDIR)/*.o $(TARGET) $(BENCHMARK_TARGET)

run: $(TARGET)
	./$(TARGET)
