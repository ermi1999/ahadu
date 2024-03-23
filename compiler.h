#ifndef AHADU_COMPILER_H
#define AHADU_COMPILER_H

#include <wctype.h>
#include <wchar.h>

#include "vm.h"

ObjFunction *compile(const wchar_t *source);

#endif // !AHADU_COMPILER_H
