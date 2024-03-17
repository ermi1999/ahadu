#ifndef AHADU_CHUNK_H
#define AHADU_CHUNK_H

#include "common.h"
#include "value.h"


typedef enum {
  OP_CONSTANT,
  OP_NIL,
  OP_TRUE,
  OP_FALSE,
  OP_EQUAL,
  OP_GREATER,
  OP_LESS,
  OP_ADD,
  OP_SUBTRACT,
  OP_MULTIPLY,
  OP_DIVIDE,
  OP_NOT,
  OP_NEGATE,
  OP_RETURN,
} OpCode;

// a dynamic array to store some data along with the bytecode instruction
typedef struct {
  int count; // how many in use
  int capacity; // the number of elements in the arrays allocated 
  uint8_t *code;
  int *lines; // to store the line number
  ValueArray constants; // to store the constants (it has the same structure as Chunk)
} Chunk;

void initChunk(Chunk *chunk);
void freeChunk(Chunk *chunk);
void writeChunk(Chunk *chunk, uint8_t byte, int line); // to append to the chunk array
int addConstant(Chunk *chunk, Value value);

#endif
