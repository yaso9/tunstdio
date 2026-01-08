# Makefile for tunstdio and test suite

CC ?= gcc
CFLAGS ?= -Wall -Wextra -Werror -std=c11 -g -O2
CFLAGS += $(EXTRA_CFLAGS)
TEST_CFLAGS = -Wall -Wextra -Werror -std=c11 -g -O1 -fsanitize=address -fsanitize=undefined
LDFLAGS = $(EXTRA_LDFLAGS)

# Main program
MAIN_SRC = tunstdio.c
MAIN_BIN = tunstdio
MAIN_BIN_TEST = tunstdio_test

# Library (extracted testable functions)
LIB_SRC = tunstdio_lib.c
LIB_HDR = tunstdio_lib.h
LIB_OBJ = tunstdio_lib.o

# Test sources
TEST_DIR = tests
TEST_HARNESS = $(TEST_DIR)/test_harness.h
TEST_BINS = test_hex_codec test_ip_parsing test_integration

# Default target
.PHONY: all
all: $(MAIN_BIN)

# Main program (now depends on library)
$(MAIN_BIN): $(MAIN_SRC) $(LIB_OBJ) $(LIB_HDR)
	$(CC) $(CFLAGS) -o $@ $(MAIN_SRC) $(LIB_OBJ) $(LDFLAGS)

# Main program with test flags for integration tests
$(MAIN_BIN_TEST): $(MAIN_SRC) $(LIB_OBJ) $(LIB_HDR)
	$(CC) $(TEST_CFLAGS) -o $@ $(MAIN_SRC) $(LIB_OBJ) $(LDFLAGS)

# Library object
$(LIB_OBJ): $(LIB_SRC) $(LIB_HDR)
	$(CC) $(CFLAGS) -c -o $@ $(LIB_SRC)

# Test targets
.PHONY: tests
tests: $(MAIN_BIN) $(LIB_OBJ) $(TEST_BINS)

test_hex_codec: $(TEST_DIR)/test_hex_codec.c $(LIB_OBJ) $(TEST_HARNESS)
	$(CC) $(TEST_CFLAGS) -o $@ $(TEST_DIR)/test_hex_codec.c $(LIB_OBJ) $(LDFLAGS)

test_ip_parsing: $(TEST_DIR)/test_ip_parsing.c $(LIB_OBJ) $(TEST_HARNESS)
	$(CC) $(TEST_CFLAGS) -o $@ $(TEST_DIR)/test_ip_parsing.c $(LIB_OBJ) $(LDFLAGS)

test_integration: $(TEST_DIR)/test_integration.c $(LIB_OBJ) $(TEST_HARNESS) $(MAIN_BIN_TEST)
	$(CC) $(TEST_CFLAGS) -DTUNSTDIO_BIN=\"./$(MAIN_BIN_TEST)\" -o $@ $(TEST_DIR)/test_integration.c $(LIB_OBJ) $(LDFLAGS)

# Run all tests
.PHONY: check
check: tests
	@echo ""
	@echo "========================================"
	@echo "Running Unit Tests"
	@echo "========================================"
	@./test_hex_codec || exit 1
	@./test_ip_parsing || exit 1
	@echo ""
	@echo "========================================"
	@echo "Running Integration Tests"
	@echo "========================================"
	@./test_integration
	@echo ""
	@echo "========================================"
	@echo "All tests completed!"
	@echo "========================================"

# Run only unit tests (no root required)
.PHONY: check-unit
check-unit: test_hex_codec test_ip_parsing
	@echo ""
	@echo "========================================"
	@echo "Running Unit Tests Only"
	@echo "========================================"
	@./test_hex_codec || exit 1
	@./test_ip_parsing || exit 1
	@echo ""
	@echo "Unit tests completed!"

# Run integration tests (may require root for TUN tests)
.PHONY: check-integration
check-integration: test_integration $(MAIN_BIN_TEST)
	@echo ""
	@echo "========================================"
	@echo "Running Integration Tests"
	@echo "========================================"
	@./test_integration

# Run tests with root (for full integration testing)
.PHONY: check-root
check-root: tests
	@echo ""
	@echo "Running tests with sudo..."
	@sudo ./test_hex_codec
	@sudo ./test_ip_parsing
	@sudo ./test_integration

# Clean build artifacts
.PHONY: clean
clean:
	rm -f $(MAIN_BIN) $(MAIN_BIN_TEST) $(LIB_OBJ) $(TEST_BINS)
	rm -f *.o

# Install (optional)
.PHONY: install
install: $(MAIN_BIN)
	install -m 755 $(MAIN_BIN) /usr/local/bin/

# Uninstall
.PHONY: uninstall
uninstall:
	rm -f /usr/local/bin/$(MAIN_BIN)

# Help
.PHONY: help
help:
	@echo "Available targets:"
	@echo "  all              - Build main program (default)"
	@echo "  tests            - Build all tests"
	@echo "  check            - Run all tests"
	@echo "  check-unit       - Run unit tests only (no root required)"
	@echo "  check-integration - Run integration tests"
	@echo "  check-root       - Run all tests with sudo (for TUN tests)"
	@echo "  clean            - Remove build artifacts"
	@echo "  install          - Install to /usr/local/bin"
	@echo "  uninstall        - Remove from /usr/local/bin"
	@echo "  help             - Show this help"
