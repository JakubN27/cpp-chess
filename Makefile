# Chess Game Makefile

# Compiler and flags
CXX = g++
CXXFLAGS = -fdiagnostics-color=always -g -I./include -std=c++20
LDFLAGS = -lcurl

# SFML (GUI) flags
SFML_PREFIX = $(shell brew --prefix sfml 2>/dev/null)
SFML_CPPFLAGS = -I$(SFML_PREFIX)/include
SFML_LDFLAGS = -L$(SFML_PREFIX)/lib
SFML_LIBS = -lsfml-graphics -lsfml-window -lsfml-system

# Directories
SRC_DIR = src
INCLUDE_DIR = include
TEST_DIR = tests
BUILD_DIR = build
BIN_DIR = bin

# Source files
SOURCES = $(SRC_DIR)/main.cpp $(SRC_DIR)/gamestate.cpp $(SRC_DIR)/board.cpp $(SRC_DIR)/piece.cpp $(SRC_DIR)/bot.cpp $(SRC_DIR)/cli.cpp $(SRC_DIR)/algebraic.cpp $(SRC_DIR)/lichess.cpp $(TEST_DIR)/test.cpp
OBJECTS = $(SOURCES:%.cpp=$(BUILD_DIR)/%.o)

# GUI source files
GUI_SOURCES = $(SRC_DIR)/gui_main.cpp $(SRC_DIR)/gamestate.cpp $(SRC_DIR)/board.cpp $(SRC_DIR)/piece.cpp $(SRC_DIR)/bot.cpp $(SRC_DIR)/cli.cpp $(SRC_DIR)/algebraic.cpp $(SRC_DIR)/lichess.cpp
GUI_OBJECTS = $(GUI_SOURCES:%.cpp=$(BUILD_DIR)/%.o)
GUI_TARGET = $(BIN_DIR)/gui

# Output executable
TARGET = $(BIN_DIR)/chess

# Phony targets
.PHONY: all clean run help build gui

# Default target
all: build

# Build the project
build: $(TARGET)

# Build the SFML GUI
gui: $(GUI_TARGET)

$(GUI_TARGET): $(GUI_OBJECTS)
	@mkdir -p $(BIN_DIR)
	$(CXX) $(CXXFLAGS) $(SFML_CPPFLAGS) $(GUI_OBJECTS) $(SFML_LDFLAGS) $(SFML_LIBS) -o $(GUI_TARGET)
	@echo "Build complete! Executable: $(GUI_TARGET)"

# Create executable
$(TARGET): $(OBJECTS)
	@mkdir -p $(BIN_DIR)
	$(CXX) $(CXXFLAGS) $(OBJECTS) $(LDFLAGS) -o $(TARGET)
	@echo "Build complete! Executable: $(TARGET)"

# Compile source files to object files
$(BUILD_DIR)/%.o: %.cpp
	@mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) $(SFML_CPPFLAGS) -c $< -o $@

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
	@echo "  build     - Build the console/test executable"
	@echo "  gui       - Build the SFML GUI executable"
	@echo "  run       - Build and run the console/test executable"
	@echo "  clean     - Remove build artifacts"
	@echo "  help      - Display this help message"
	@echo ""
	@echo "Example:"
	@echo "  make build   - Compile console/test binary"
	@echo "  make gui     - Compile SFML GUI binary"
	@echo "  make run     - Compile and run console/test binary"
	@echo "  make clean   - Remove build files"
