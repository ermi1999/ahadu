#ifndef AHADU_CHUNK_H
#define AHADU_CHUNK_H

#include "common.h"
#include "value.h"


typedef enum {
  OP_CONSTANT,
  OP_RETURN,
} OpCode;

// a dynamic array to store some data along with the bytecode instruction
typedef struct {
  int count; // how many in use
  int capacity; // the number of elements in the arrays allocated 
  uint8_t *code;
  ValueArray constants; // to store the constants (it has the same structure as Chunk)
} Chunk;

void initChunk(Chunk *chunk);
void freeChunk(Chunk *chunk);
void writeChunk(Chunk *chunk, uint8_t byte); // to append to the chunk array
int addConstant(Chunk *chunk, Value value);

#endif
