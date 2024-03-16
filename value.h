#ifndef AHADU_VALUE_H
#define AHADU_VALUE_H

#include "common.h"

typedef enum {
  VAL_BOOL,
  VAL_NIL,
  VAL_NUMBER,
} ValueType;

typedef double Value;

/**
 * array definition for constant value array.
 * @capacity: the capacity of the array
 * @count: currently used spots from the capacity.
 * @values: array values;
 */
typedef struct {
  int capacity;
  int count;
  Value *values;
} ValueArray;

void initValueArray(ValueArray *array);
void writeValueArray(ValueArray *array, Value value);
void freeValueArray(ValueArray *array);
void printValue(Value value);

#endif
