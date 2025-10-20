CXX = g++
CXXFLAGS = -Wall -Wextra -std=c++11 -g
TARGET = server
SRC_DIR = src
OBJ_DIR = obj

#source files
SOURCES = $(wildcard $(SRC_DIR)/*.cpp)
OBJECTS = $(SOURCES:$(SRC_DIR)/%.cpp=$(OBJ_DIR)/%.o)

#default target
all: $(TARGET)

$(OBJ_DIR): #create if it doesn't exist
	mkdir -p $(OBJ_DIR)

#link object files to create executable
$(TARGET): $(OBJECTS)
	$(CXX) $(CXXFLAGS) -o $@ $^
	@echo "Build complete: $(TARGET)"

#compile source files to object files
$(OBJ_DIR)/%.o: $(SRC_DIR)/%.cpp | $(OBJ_DIR)
	$(CXX) $(CXXFLAGS) -c $< -o $@

#clean build artifacts
clean:
	rm -rf $(OBJ_DIR) $(TARGET)
	@echo "Clean complete"

#run server
run: $(TARGET)
	./$(TARGET)

#rebuild everything
rebuild: clean all

.PHONY: all clean run rebuild