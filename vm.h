#ifndef AHADU_VM_H
#define AHADU_VM_H

#include <wctype.h>
#include <wchar.h>

#include "chunk.h"
#include "value.h"

#define STACK_MAX 256

/**
 * @chunk: the chunk that the vm executs
 * @ip: the location of the current instruction.
 */
typedef struct {
  Chunk *chunk;
  uint8_t *ip;
  Value stack[STACK_MAX];
  Value *stackTop;
  Obj *objects;
} VM;

/**
 * InterpretResult - defines the interpretion process results.
 */
typedef enum {
  INTERPRET_OK,
  INTERPRET_COMPILE_ERROR,
  INTERPRET_RUNTIME_ERROR
} InterpretResult;

extern VM vm;

void initVM();
void freeVM();
InterpretResult interpret(const wchar_t *source);
void push(Value value);
Value pop();

#endif
