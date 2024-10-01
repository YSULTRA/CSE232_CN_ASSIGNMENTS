# Makefile for compiling and running the server

# Compiler
CC = gcc

# Compiler flags
CFLAGS = -pthread -Wall

# Source file
SRCS = server.c

# Output executable
TARGET = server

# Default target
all: $(TARGET)

# Compile the server
$(TARGET): $(SRCS)
	$(CC) $(CFLAGS) -o $(TARGET) $(SRCS)

# Run the server with taskset
run: $(TARGET)
	taskset -c 0 ./$(TARGET)

# Clean up generated files
clean:
	rm -f $(TARGET)
