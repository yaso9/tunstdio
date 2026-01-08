#!/bin/bash
#
# Test runner script for tunstdio
#
# Usage:
#   ./tests/run_tests.sh [options]
#
# Options:
#   -u, --unit         Run only unit tests
#   -i, --integration  Run only integration tests
#   -a, --all          Run all tests (default)
#   -r, --root         Run with sudo (for TUN device tests)
#   -v, --verbose      Verbose output
#   -h, --help         Show help
#

set -e

# Colors
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[0;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

# Script directory
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_DIR="$(dirname "$SCRIPT_DIR")"

# Options
RUN_UNIT=1
RUN_INTEGRATION=1
USE_SUDO=0
VERBOSE=0

# Parse arguments
while [[ $# -gt 0 ]]; do
    case $1 in
        -u|--unit)
            RUN_UNIT=1
            RUN_INTEGRATION=0
            shift
            ;;
        -i|--integration)
            RUN_UNIT=0
            RUN_INTEGRATION=1
            shift
            ;;
        -a|--all)
            RUN_UNIT=1
            RUN_INTEGRATION=1
            shift
            ;;
        -r|--root)
            USE_SUDO=1
            shift
            ;;
        -v|--verbose)
            VERBOSE=1
            shift
            ;;
        -h|--help)
            echo "Usage: $0 [options]"
            echo ""
            echo "Options:"
            echo "  -u, --unit         Run only unit tests"
            echo "  -i, --integration  Run only integration tests"
            echo "  -a, --all          Run all tests (default)"
            echo "  -r, --root         Run with sudo (for TUN device tests)"
            echo "  -v, --verbose      Verbose output"
            echo "  -h, --help         Show help"
            exit 0
            ;;
        *)
            echo "Unknown option: $1"
            exit 1
            ;;
    esac
done

# Change to project directory
cd "$PROJECT_DIR"

# Build
echo -e "${BLUE}Building tests...${NC}"
if [[ $VERBOSE -eq 1 ]]; then
    make tests
else
    make tests > /dev/null 2>&1
fi
echo -e "${GREEN}Build complete${NC}"
echo ""

# Track results
TESTS_PASSED=0
TESTS_FAILED=0
TESTS_SKIPPED=0

run_test() {
    local test_name=$1
    local test_bin=$2
    
    echo -e "${BLUE}Running $test_name...${NC}"
    
    local runner=""
    if [[ $USE_SUDO -eq 1 ]]; then
        runner="sudo"
    fi
    
    local result=0
    if [[ $VERBOSE -eq 1 ]]; then
        $runner ./$test_bin || result=$?
    else
        $runner ./$test_bin 2>&1 | tail -n 10 || result=$?
    fi
    
    if [[ $result -eq 0 ]]; then
        echo -e "${GREEN}$test_name: PASSED${NC}"
        ((TESTS_PASSED++))
    elif [[ $result -eq 77 ]]; then
        echo -e "${YELLOW}$test_name: SKIPPED${NC}"
        ((TESTS_SKIPPED++))
    else
        echo -e "${RED}$test_name: FAILED${NC}"
        ((TESTS_FAILED++))
    fi
    echo ""
}

# Run unit tests
if [[ $RUN_UNIT -eq 1 ]]; then
    echo ""
    echo -e "${YELLOW}========================================${NC}"
    echo -e "${YELLOW}UNIT TESTS${NC}"
    echo -e "${YELLOW}========================================${NC}"
    
    run_test "Hex Codec Tests" "test_hex_codec"
    run_test "IP Parsing Tests" "test_ip_parsing"
fi

# Run integration tests
if [[ $RUN_INTEGRATION -eq 1 ]]; then
    echo ""
    echo -e "${YELLOW}========================================${NC}"
    echo -e "${YELLOW}INTEGRATION TESTS${NC}"
    echo -e "${YELLOW}========================================${NC}"
    
    if [[ $USE_SUDO -eq 0 ]]; then
        echo -e "${YELLOW}Note: Running without root. TUN tests will be skipped.${NC}"
        echo -e "${YELLOW}Use -r flag to run with sudo for full test coverage.${NC}"
        echo ""
    fi
    
    run_test "Integration Tests" "test_integration"
fi

# Summary
echo ""
echo -e "${YELLOW}========================================${NC}"
echo -e "${YELLOW}SUMMARY${NC}"
echo -e "${YELLOW}========================================${NC}"
echo -e "Passed:  ${GREEN}$TESTS_PASSED${NC}"
echo -e "Failed:  ${RED}$TESTS_FAILED${NC}"
echo -e "Skipped: ${YELLOW}$TESTS_SKIPPED${NC}"
echo ""

if [[ $TESTS_FAILED -gt 0 ]]; then
    echo -e "${RED}Some tests failed!${NC}"
    exit 1
else
    echo -e "${GREEN}All tests passed!${NC}"
    exit 0
fi
