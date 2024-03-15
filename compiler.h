#ifndef AHADU_COMPILER_H
#define AHADU_COMPILER_H

#include <wctype.h>
#include <wchar.h>

#include "vm.h"

bool compile(const wchar_t *source, Chunk *chunk);

#endif // !AHADU_COMPILER_H
