#include <stdio.h>
#include "debug.h"

void disassebleChunk(Chunk *chunk, const char *name) {
  printf("== %s ==\n", name);

  for (int offset = 0; offset < chunk->count;) {
    offset = disassebleInstruction(chunk, offset);
  }
}

int disassebleInstruction(Chunk *chunk, int offset) {
  printf("%04d ", offset);

  uint8_t instruction = chunk->code[offset];

  switch (instruction) {
    case OP_RETURN:
      return simpleInstruction("OP_RETURN", offset);
    default:
      printf("Unknown opcode %d\n", instruction);
      return offset + 1;
  }
}

int simpleInstruction(const char *name, int offset) {
  printf("%s\n", name);
  return offset + 1;
}
