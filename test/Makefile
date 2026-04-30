# Makefile for the murm test suite.
#
# Targets:
#   test          (default) Build and run every test binary; non-zero exit
#                 status if any test fails.
#   build         Build every test binary, but don't run them.
#   clean         Remove the build/test directory.
#   <test_name>   Build and run just one test binary, e.g. `make test_port`.
#
# Layout: each test/test_*.cpp is built into its own binary in
#         <repo>/build/test/<name>, then executed in turn.

# Paths
REPO_ROOT := $(realpath $(dir $(lastword $(MAKEFILE_LIST)))/..)
BUILD_DIR := $(REPO_ROOT)/build/test
TEST_DIR  := $(REPO_ROOT)/test
INCLUDES  := -I$(REPO_ROOT)/include -I$(TEST_DIR)

# Compiler and flags
CXX        := g++
CXXSTD     := -std=c++17
WARNINGS   := -Wall -Wextra -Wno-sign-compare
OPT        := -O2 -g
CXXFLAGS   := $(CXXSTD) $(WARNINGS) $(OPT) $(INCLUDES)
LDFLAGS    := -fopenmp -pthread

# Discover every test_*.cpp in the test directory.
TEST_SRCS  := $(wildcard $(TEST_DIR)/test_*.cpp)
TEST_NAMES := $(notdir $(TEST_SRCS:.cpp=))
TEST_BINS  := $(addprefix $(BUILD_DIR)/, $(TEST_NAMES))

.PHONY: test build clean help $(TEST_NAMES)

# The default target builds and runs every test.
test: build
	@echo
	@echo "==================== running tests ===================="
	@fail=0; \
	for bin in $(TEST_BINS); do \
		echo; \
		echo "------ $$(basename $$bin) ------"; \
		"$$bin" || fail=$$((fail+1)); \
	done; \
	echo; \
	if [ $$fail -eq 0 ]; then \
		echo "==================== ALL TESTS PASSED ===================="; \
		exit 0; \
	else \
		echo "==================== $$fail TEST BINARY/IES FAILED ===================="; \
		exit 1; \
	fi

# Build everything but don't run.
build: $(TEST_BINS)

# Convenience phony target per test (e.g. `make test_port`).
$(TEST_NAMES): %: $(BUILD_DIR)/%
	"$<"

$(BUILD_DIR)/%: $(TEST_DIR)/%.cpp $(TEST_DIR)/test_framework.hpp $(TEST_DIR)/test_helpers.hpp | $(BUILD_DIR)
	$(CXX) $(CXXFLAGS) $(LDFLAGS) -o $@ $<

$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

clean:
	rm -rf $(BUILD_DIR)

help:
	@echo "Targets:"
	@echo "  test    (default) build and run every test binary"
	@echo "  build   build every test binary, don't run"
	@echo "  clean   remove $(BUILD_DIR)"
	@echo
	@echo "Per-test convenience targets:"
	@for n in $(TEST_NAMES); do echo "  $$n"; done
