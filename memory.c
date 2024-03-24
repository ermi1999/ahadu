#include <stdlib.h>

#include "memory.h"
#include "vm.h"

/**
 * reallocate - handles the realocation of a dynamic array.
 * @pointer: the pointer to the array.
 * @oldSize: the old size of the array.
 * @newSize: the new size of the array to be resized to.
 * return: the modified array.
 */
void *reallocate(void *pointer, size_t oldSize, size_t newSize) {
  if (newSize == 0) {
    free(pointer);
    return NULL;
  }

  // realloc handles the rest, if newSize is > oldSize it grows the size of the arrray otherwise it shrinks it.
  void *result = realloc(pointer, newSize);
  if (result == NULL)
    exit(1);
  return result;
}

/**
 * freeObject - frees an object.
 * @object: the object to be freed.
 * Return: nothing.
 */
static void freeObject(Obj *object) {
  switch (object->type) {
    case OBJ_CLOSURE:
      FREE(ObjClosure, object);
      break;
    case OBJ_FUNCTION: {
      ObjFunction *function = (ObjFunction *)object;
      freeChunk(&function->chunk);
      FREE(ObjFunction, object);
      break;
    }
    case OBJ_NATIVE:
      FREE(ObjNative, object);
      break;
    case OBJ_STRING: {
      ObjString *string = (ObjString *)object;
      FREE_ARRAY(wchar_t, string->chars, string->length + 1);
      FREE(ObjString, object);
      break;
    }
  }
}
/**
 * freeObjects - frees the objects.
 * Return: nothing.
 */
void freeObjects() {
  Obj *object = vm.objects;
  while (object != NULL) {
    Obj *next = object->next;
    freeObject(object);
    object = next;
  }
}
