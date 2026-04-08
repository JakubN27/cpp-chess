# Chess Game Makefile

# Compiler and flags
CXX = g++
CXXFLAGS = -fdiagnostics-color=always -g -I./include -std=c++20
LDFLAGS = 

# Directories
SRC_DIR = src
INCLUDE_DIR = include
TEST_DIR = tests
BUILD_DIR = build
BIN_DIR = bin

# Source files
SOURCES = $(SRC_DIR)/main.cpp $(SRC_DIR)/gamestate.cpp $(SRC_DIR)/board.cpp $(SRC_DIR)/piece.cpp $(TEST_DIR)/test.cpp
OBJECTS = $(SOURCES:%.cpp=$(BUILD_DIR)/%.o)

# Output executable
TARGET = $(BIN_DIR)/chess

# Phony targets
.PHONY: all clean run help build

# Default target
all: build

# Build the project
build: $(TARGET)

# Create executable
$(TARGET): $(OBJECTS)
	@mkdir -p $(BIN_DIR)
	$(CXX) $(CXXFLAGS) $(OBJECTS) $(LDFLAGS) -o $(TARGET)
	@echo "Build complete! Executable: $(TARGET)"

# Compile source files to object files
$(BUILD_DIR)/%.o: %.cpp
	@mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) -c $< -o $@

# Run the executable
run: $(TARGET)
	./$(TARGET)

# Clean build artifacts
clean:
	rm -rf $(BUILD_DIR) $(BIN_DIR)
	@echo "Clean complete!"

# Help message
help:
	@echo "Chess Game Makefile"
	@echo "===================="
	@echo "Usage: make [target]"
	@echo ""
	@echo "Targets:"
	@echo "  all       - Build the project (default)"
	@echo "  build     - Build the executable"
	@echo "  run       - Build and run the executable"
	@echo "  clean     - Remove build artifacts"
	@echo "  help      - Display this help message"
	@echo ""
	@echo "Example:"
	@echo "  make build   - Compile the project"
	@echo "  make run     - Compile and run"
	@echo "  make clean   - Remove build files"
