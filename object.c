#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "memory.h"
#include "object.h"
#include "table.h"
#include "value.h"
#include "vm.h"

#define ALLOCATE_OBJ(type, objectType) \
  (type *)allocateObject(sizeof(type), objectType)

static Obj *allocateObject(size_t size, ObjType type) {
  Obj *object = (Obj *)reallocate(NULL, 0, size);
  object->type = type;

  object->next = vm.objects;
  vm.objects = object;
  return object;
}

static ObjString *allocateString(wchar_t *chars, int length, uint32_t hash) {
  ObjString *string = ALLOCATE_OBJ(ObjString, OBJ_STRING);
  string->length = length;
  string->chars = chars;
  string->hash = hash;
  tableSet(&vm.strings, string, NIL_VAL);
  return string;
}

static uint32_t hashString(const wchar_t *key, int length) {
  uint32_t hash = 2166136261u;
  for (int i = 0; i < length; i++) {
    hash ^= (uint8_t)key[i];
    hash *= 16777619;
  }
  return hash;
}

ObjString *takeString(wchar_t *chars, int length) {
  uint32_t hash = hashString(chars, length);
  ObjString *interned = tableFindString(&vm.strings, chars, length, hash);
  if (interned != NULL) {
    FREE_ARRAY(wchar_t, chars, length + 1);
    return interned;
  }
  return allocateString(chars, length, hash);
}

ObjString *copyString(const wchar_t *chars, int length) {
  uint32_t hash = hashString(chars, length);
  ObjString *interned = tableFindString(&vm.strings, chars, length, hash);
  if (interned != NULL) return interned;
  wchar_t *heapChars = ALLOCATE(wchar_t, length + 1);
  wmemcpy(heapChars, chars, length);
  heapChars[length] = L'\0';
  return allocateString(heapChars, length, hash);
}

void printObject(Value value) {
  switch (OBJ_TYPE(value)) {
    case OBJ_STRING:
      printf("%ls", AS_STRING(value)->chars);
      break;
  }
}