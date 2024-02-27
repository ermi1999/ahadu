#ifndef AHADU_CHUNK_H
#define AHADU_CHUNK_H

#include "common.h"


typedef enum {
  OP_RETURN,
} OpCode;

// a dynamic array to store some data along with the bytecode instruction
typedef struct {
  int count; // how many in use
  int capacity; // the number of elements in the arrays allocated 
  uint8_t *code;
} Chunk;

void initChunk(Chunk *chunk);
void writeChunk(Chunk *chunk, uint8_t byte); // to append to the chunk array

#endif
