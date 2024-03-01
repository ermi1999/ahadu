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
}

void freeChunk(Chunk *chunk) {
  FREE_ARRAY(uint8_t, chunk->code, chunk->capacity);
  initChunk(chunk);
}

/**
 * writeChunk - to append a byte to chunk array
 * @chunk: the chunk array that was intialized by initChunk
 * @byte: the new byte to be written.
 * Return: nothing.
 */
void writeChunk(Chunk *chunk, uint8_t byte) {
  if (chunk->capacity < chunk->count + 1) {
    int oldCapacity = chunk->capacity;
    chunk->capacity = GROW_CAPACITY(oldCapacity);
    chunk->code = GROW_ARRAY(uint8_t, chunk->code,
        oldCapacity, chunk->capacity);
  }

  chunk->code[chunk->count] = byte;
  chunk->count++;
}
