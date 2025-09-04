TARGET = bin/ahadu
SRC = $(wildcard src/*.c)
OBJ = $(patsubst src/%.c, obj/%.o, $(SRC))

CC = gcc
CFLAGS = -Iinclude -Wall
LDFLAGS = 
DEBUG_FLAGS = -g -O0

# Default build
.PHONY: all clean run debug valgrind

all: $(TARGET)

# Build target
$(TARGET): $(OBJ)
	@mkdir -p bin
	$(CC) $(LDFLAGS) -o $@ $^

# Compile objects
obj/%.o: src/%.c
	@mkdir -p obj
	$(CC) -c $< -o $@ $(CFLAGS)

# Clean
clean:
	rm -rf bin obj

# Run the program
run: $(TARGET)
	./$(TARGET)

# Debug build
debug: CFLAGS += $(DEBUG_FLAGS)
debug: clean all

# Run with valgrind
valgrind: debug
	valgrind --leak-check=full --show-leak-kinds=all ./$(TARGET)