CXX := g++
CXXFLAGS := -std=c++20 -O3 -march=native -Wall -Wextra -Wpedantic -fopenmp
LDFLAGS := -fopenmp

TARGET := sorting_networks
SRCDIR := src
SOURCES := $(wildcard $(SRCDIR)/*.cpp)
OBJECTS := $(SOURCES:.cpp=.o)

.PHONY: all clean release debug profile run

all: release

release: CXXFLAGS += -DNDEBUG
release: $(TARGET)

debug: CXXFLAGS := -std=c++20 -g -O0 -Wall -Wextra -Wpedantic -fopenmp
debug: $(TARGET)

profile: CXXFLAGS += -DENABLE_PROFILING
profile: $(TARGET)

$(TARGET): $(OBJECTS)
	$(CXX) $(OBJECTS) -o $@ $(LDFLAGS)

$(SRCDIR)/%.o: $(SRCDIR)/%.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

clean:
	rm -f $(OBJECTS) $(TARGET)

run: $(TARGET)
	./$(TARGET)
