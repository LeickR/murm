
# Paths
REPO_ROOT := $(realpath $(dir $(lastword $(MAKEFILE_LIST)))/../..)
BUILD_DIR := $(REPO_ROOT)/build
TARGET    := $(BUILD_DIR)/example1
SRC       := $(REPO_ROOT)/examples/example1/example1.cpp

# Compiler and flags
CXX      := g++
CXXFLAGS := -std=c++17 -Wall -Wextra -I$(REPO_ROOT)/include
LDFLAGS  := -fopenmp -pthread

# Default: debug build
DEBUG_FLAGS   := -O0 -g
RELEASE_FLAGS := -O2 -DNDEBUG

.PHONY: all debug release run clean

all: release

debug: CXXFLAGS += $(DEBUG_FLAGS)
debug: $(TARGET)

release: CXXFLAGS += $(RELEASE_FLAGS)
release: $(TARGET)

$(TARGET): $(SRC) | $(BUILD_DIR)
	$(CXX) $(CXXFLAGS) $(LDFLAGS) -o $@ $<

$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

run: release
	$(TARGET)

clean:
	rm -f $(TARGET)
