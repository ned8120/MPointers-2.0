# Compiler and flags
CXX = g++
CXXFLAGS = -std=c++17 -Wall -Wextra -pthread
LDFLAGS = -pthread

# Directories
SRC_DIR = src
INCLUDE_DIR = include
BUILD_DIR = build
BIN_DIR = bin

# Memory Manager
MM_SRC_DIR = $(SRC_DIR)/MemoryManager
MM_SRCS = $(MM_SRC_DIR)/main.cpp $(MM_SRC_DIR)/MemoryManager.cpp
MM_OBJS = $(patsubst $(SRC_DIR)/%.cpp,$(BUILD_DIR)/%.o,$(MM_SRCS))
MM_BIN = $(BIN_DIR)/MemoryManager

# MPointers
MP_SRC_DIR = $(SRC_DIR)/MPointers
MP_SRCS = $(MP_SRC_DIR)/test.cpp
MP_OBJS = $(patsubst $(SRC_DIR)/%.cpp,$(BUILD_DIR)/%.o,$(MP_SRCS))
MP_BIN = $(BIN_DIR)/TestMPointers

# Test
TEST_SRC_DIR = $(SRC_DIR)/Test
TEST_SRCS = $(wildcard $(TEST_SRC_DIR)/*.cpp)
TEST_OBJS = $(patsubst $(SRC_DIR)/%.cpp,$(BUILD_DIR)/%.o,$(TEST_SRCS))
TEST_BIN = $(BIN_DIR)/Test

# All targets
all: directories $(MM_BIN) $(MP_BIN) $(TEST_BIN)

# Create necessary directories
directories:
	mkdir -p $(BUILD_DIR)/MemoryManager
	mkdir -p $(BUILD_DIR)/MPointers
	mkdir -p $(BUILD_DIR)/Test
	mkdir -p $(BIN_DIR)
	mkdir -p dump_files

# Memory Manager
$(MM_BIN): $(MM_OBJS)
	$(CXX) $(LDFLAGS) -o $@ $^

# MPointers
$(MP_BIN): $(MP_OBJS)
	$(CXX) $(LDFLAGS) -o $@ $^

# Test
$(TEST_BIN): $(TEST_OBJS)
	$(CXX) $(LDFLAGS) -o $@ $^

# Compile rule
$(BUILD_DIR)/%.o: $(SRC_DIR)/%.cpp
	mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) -I$(INCLUDE_DIR) -c -o $@ $<

# Clean rule
clean:
	rm -rf $(BUILD_DIR)
	rm -rf $(BIN_DIR)

# Run Memory Manager
run-mm: $(MM_BIN)
	$(MM_BIN) 8080 10 dump_files

# Run MPointers test
run-mp: $(MP_BIN)
	$(MP_BIN)

.PHONY: all directories clean run-mm run-mp