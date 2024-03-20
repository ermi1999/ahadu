#include <stdio.h>
#include "debug.h"
#include "value.h"

/**
  * disassembleChunk - disassembles a chunk.
  * @chunk: the chunk to be disassembled.
  * @name: the name of the chunk.
  * Return: nothing.
  */
void disassembleChunk(Chunk *chunk, const char *name) {
  printf("== %s ==\n", name);

  for (int offset = 0; offset < chunk->count;) {
    offset = disassembleInstruction(chunk, offset);
  }
}

/**
  * simpleInstruction - disassembles a simple instruction.
  * @name: the name of the instruction.
  * @offset: the offset of the current instruction.
  * Return: the offset of the next instruction.
  */
static int simpleInstruction(const char *name, int offset) {
  printf("%s\n", name);
  return offset + 1;
}

static int byteInstruction(const char *name, Chunk *chunk, int offset) {
  uint8_t slot = chunk->code[offset + 1];
  printf("%-16s %4d\n", name, slot);
  return offset + 2;
}

/**
  * constantInstruction - disassembles a constant instruction.
  * @name: the name of the instruction.
  * @chunk: the chunk to be disassembled.
  * @offset: the offset of the current instruction.
  * Return: the offset of the next instruction.
  */
static int constantInstruction(const char *name, Chunk *chunk, int offset) {
  uint8_t constant = chunk->code[offset + 1];
  printf("%-16s %4d '", name, constant);
  printValue(chunk->constants.values[constant]);
  printf("'\n");
  return offset + 2;
}

/**
  * disassembleInstruction - disassembles an instruction.
  * @chunk: the chunk to be disassembled.
  * @offset: the offset of the current instruction.
  * Return: the offset of the next instruction.
  */
int disassembleInstruction(Chunk *chunk, int offset) {
  printf("%04d ", offset);
  if (offset > 0 &&
		  chunk->lines[offset] == chunk->lines[offset - 1]) {
	  printf("   | ");
  } else {
	  printf("%4d ", chunk->lines[offset]);
  }

  uint8_t instruction = chunk->code[offset];

  switch (instruction) {
    case OP_RETURN:
      return simpleInstruction("OP_RETURN", offset);
    case OP_CONSTANT:
      return constantInstruction("OP_CONSTANT", chunk, offset);
    case OP_NIL:
      return simpleInstruction("OP_NIL", offset);
    case OP_TRUE:
      return simpleInstruction("OP_TRUE", offset);
    case OP_FALSE:
      return simpleInstruction("OP_FALSE", offset);
    case OP_POP:
      return simpleInstruction("OP_POP", offset);
    case OP_GET_LOCAL:
      return simpleInstruction("OP_GET_LOCAL", offset);
    case OP_SET_LOCAL:
      return simpleInstruction("OP_SET_LOCAL", offset);
    case OP_GET_GLOBAL:
      return constantInstruction("OP_GET_GLOBAL", chunk, offset);
    case OP_DEFINE_GLOBAL:
      return constantInstruction("OP_DEFINE_GLOBAL", chunk, offset);
    case OP_SET_GLOBAL:
      return constantInstruction("OP_SET_GLOBAL", chunk, offset);
    case OP_EQUAL:
      return simpleInstruction("OP_EQUAL", offset);
    case OP_GREATER:
      return simpleInstruction("OP_GREATER", offset);
    case OP_LESS:
      return simpleInstruction("OP_LESS", offset);
    case OP_ADD:
      return simpleInstruction("OP_ADD", offset);
    case OP_SUBTRACT:
      return simpleInstruction("OP_SUBTRACT", offset);
    case OP_MULTIPLY:
      return simpleInstruction("OP_MULTIPLY", offset);
    case OP_DIVIDE:
      return simpleInstruction("OP_DIVIDE", offset);
    case OP_NOT:
      return simpleInstruction("OP_NOT", offset);
    case OP_NEGATE:
      return simpleInstruction("OP_NEGATE", offset);
    case OP_PRINT:
      return simpleInstruction("OP_PRINT", offset);
    default:
      printf("Unknown opcode %d\n", instruction);
      return offset + 1;
  }
}

