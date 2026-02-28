#!/bin/bash

# Benchmark script for tracking performance during refactoring
# Usage: ./benchmark.sh [phase_name]

set -e

PHASE="${1:-baseline}"
LOG_DIR="logs"
TIMESTAMP=$(date +%Y%m%d_%H%M%S)
mkdir -p "$LOG_DIR"

echo "=========================================="
echo "Running benchmarks for: $PHASE"
echo "Timestamp: $TIMESTAMP"
echo "=========================================="

# Build release
echo "Building release..."
make clean > /dev/null 2>&1
make release > /dev/null 2>&1

# Run benchmarks
echo ""
echo "Running n=13, b=1000, t=10..."
timeout 120 ./sorting_networks -n 13 -b 1000 -t 10 2>&1 | tee "$LOG_DIR/${PHASE}_n13_${TIMESTAMP}.log"

echo ""
echo "Running n=14, b=1000, t=10..."
timeout 120 ./sorting_networks -n 14 -b 1000 -t 10 2>&1 | tee "$LOG_DIR/${PHASE}_n14_${TIMESTAMP}.log"

echo ""
echo "=========================================="
echo "Benchmarks complete for: $PHASE"
echo "Logs saved to: $LOG_DIR/"
echo "=========================================="
