#include <stdio.h>
#include <string.h>

#include "object.h"
#include "memory.h"
#include "value.h"

/**
 * initValueArray - initializes a value array.
 * @array: a pointer to a value array.
 * Return: Nothing.
 */
void initValueArray(ValueArray *array) {
  array->values = NULL;
  array->capacity = 0;
  array->count = 0;
}

/**
 * writeValueArray - writes a value in to a value array. it also handles dynamicaly allocating the array if needed.
 * @array: a pointer to a value array.
 * @value: the value to be added.
 * Return: Nothing.
 */
void writeValueArray(ValueArray *array, Value value) {
  if (array->capacity  < array->count + 1) {
    int oldCapacity = array->capacity;
    array->capacity = GROW_CAPACITY(oldCapacity);
    array->values = GROW_ARRAY(Value, array->values, oldCapacity, array->capacity);
  }

  array->values[array->count] = value;
  array->count++;
}

/**
 * freeValueArray - frees a value array.
 * @array: the value array to be freed.
 * Return: Nothing.
 */
void freeValueArray(ValueArray *array) {
  FREE_ARRAY(Value, array->values, array->capacity);
  initValueArray(array);
}


void printValue(Value value) {
  switch (value.type) {
    case VAL_BOOL:
      printf(AS_BOOL(value) ? "true" : "false");
      break;
    case VAL_NIL: printf("nil"); break;
    case VAL_NUMBER: printf("%g", AS_NUMBER(value)); break;
    case VAL_OBJ: printObject(value);
  }
}

bool valuesEqual(Value a, Value b) {
  if (a.type != b.type) return false;
  switch (a.type) {
    case VAL_BOOL: return AS_BOOL(a) == AS_BOOL(b);
    case VAL_NIL: return true;
    case VAL_NUMBER: return AS_NUMBER(a) == AS_NUMBER(b);
    case VAL_OBJ: {
      ObjString *aString = AS_STRING(a);
      ObjString *bString = AS_STRING(b);
      return aString->length == bString->length &&
        memcmp(aString->chars, bString->chars, aString->length) == 0;
    }
    default: return false; // Unreachable.
  }
}
