#include "chunk.h"
#include "memory.h"
#include <stdlib.h>
/**
 * initChunk - intializes the chunk dynamic array.
 * @chunk: a pointer to the array.
 * Return: nothing.
 */
void initChunk(Chunk *chunk) {
	chunk->count = 0;
	chunk->capacity = 0;
	chunk->code = NULL;
	chunk->lines = NULL;
	initValueArray(&chunk->constants);
}

void freeChunk(Chunk *chunk) {
	FREE_ARRAY(uint8_t, chunk->code, chunk->capacity);
	FREE_ARRAY(int, chunk->lines, chunk->capacity);
	freeValueArray(&chunk->constants);
	initChunk(chunk);
}

/**
 * writeChunk - to append a byte to chunk array
 * @chunk: the chunk array that was intialized by initChunk
 * @byte: the new byte to be written.
 * Return: nothing.
 */
void writeChunk(Chunk *chunk, uint8_t byte, int line) {
	if (chunk->capacity < chunk->count + 1) {
		int oldCapacity = chunk->capacity;
		chunk->capacity = GROW_CAPACITY(oldCapacity);
		chunk->code = GROW_ARRAY(uint8_t, chunk->code,
				oldCapacity, chunk->capacity);
		chunk->lines = GROW_ARRAY(int, chunk->lines,
				oldCapacity, chunk->capacity);
	}

	chunk->code[chunk->count] = byte;
	chunk->lines[chunk->count] = line;
	chunk->count++;
}

/**
 * addConstant - adds a new constant to the constants array in the chunk
 * @chunk: the chunk.
 * @value: the value that is going to be added to the constants array.
 * Return: the index of the newly added constant.
 */
int addConstant(Chunk* chunk, Value value) {
	writeValueArray(&chunk->constants, value);
	return chunk->constants.count - 1;
}
