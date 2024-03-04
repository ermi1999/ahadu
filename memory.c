#include "memory.h"
#include <stdlib.h>

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
