# Makefile for Plane Around Planet

CXX = g++
CXXFLAGS = -std=c++11 -Wall -I./include
LDFLAGS = -lglfw -lGL -ldl -pthread -lm

# Source files
SOURCES = main.cpp glad.c
OBJECTS = $(SOURCES:.cpp=.o) $(filter %.o,$(SOURCES:.c=.o))
EXECUTABLE = plane_viewer

# Default target
all: $(EXECUTABLE)

$(EXECUTABLE): $(OBJECTS)
	$(CXX) $(OBJECTS) -o $@ $(LDFLAGS)

# Compile C++ files
%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

# Compile C files (for glad.c)
%.o: %.c
	$(CC) -I./include -c $< -o $@

# Clean build files
clean:
	rm -f $(OBJECTS) $(EXECUTABLE)

# Run the program
run: $(EXECUTABLE)
	./$(EXECUTABLE)

.PHONY: all clean run