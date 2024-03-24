#ifndef AHADU_VM_H
#define AHADU_VM_H

#include <wctype.h>
#include <wchar.h>

#include "object.h"
#include "table.h"
#include "value.h"

#define FRAMES_MAX 64
#define STACK_MAX (FRAMES_MAX * UINT8_COUNT)

typedef struct {
  ObjClosure *closure;
  uint8_t *ip;
  Value *slots;
} CallFrame;

/**
 * @chunk: the chunk that the vm executs
 * @ip: the location of the current instruction.
 */
typedef struct {
  CallFrame frames[FRAMES_MAX];
  int frameCount;

  Value stack[STACK_MAX];
  Value *stackTop;
  Table globals;
  Table strings;
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
