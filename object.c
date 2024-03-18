#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "memory.h"
#include "object.h"
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

static ObjString *allocateString(wchar_t *chars, int length) {
  ObjString *string = ALLOCATE_OBJ(ObjString, OBJ_STRING);
  string->length = length;
  string->chars = chars;
  return string;
}

ObjString *takeString(wchar_t *chars, int length) {
  return allocateString(chars, length);
}

ObjString *copyString(const wchar_t *chars, int length) {
  wchar_t *heapChars = ALLOCATE(wchar_t, length + 1);
  wmemcpy(heapChars, chars, length);
  heapChars[length] = L'\0';
  return takeString(heapChars, length);
}

void printObject(Value value) {
  switch (OBJ_TYPE(value)) {
    case OBJ_STRING:
      printf("%ls", AS_STRING(value)->chars);
      break;
  }
}
