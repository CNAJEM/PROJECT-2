# Makefile for compiling chash.c into an executable named chash

CC = gcc
CFLAGS = -Wall -pthread

# Target executable
TARGET = chash

# Source files
SRC = chash.c

# Build target
$(TARGET): $(SRC)
	$(CC) $(CFLAGS) -o $(TARGET) $(SRC)

# Clean up
clean:
	rm -f $(TARGET)
