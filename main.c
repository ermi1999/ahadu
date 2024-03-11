#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <wctype.h>
#include <wchar.h>
#include <locale.h>

#include "common.h"
#include "chunk.h"
#include "debug.h"
#include "vm.h"

static void repl() {
  wchar_t line[1024];
  for (;;) {
    printf("> ");

    if (!fgetws(line, sizeof(line), stdin)) {
      printf("\n");
      break;
    }
    interpret(line);
  }
}

static wchar_t *readFile(const char *path) {
  FILE *file = fopen(path, "rb");
  if (file == NULL) {
    fprintf(stderr, "Could not open file \"%s\".\n", path);
    exit(74);
  }

  fseek(file, 0L, SEEK_END);
  long fileSize = ftell(file);
  rewind(file);

  char *utf8Buffer = (char*)malloc(fileSize + 1);
  if (utf8Buffer == NULL) {
    fprintf(stderr, "Not enough memory to read \"%s\".\n", path);
    exit(74);
  }

  size_t bytesRead = fread(utf8Buffer, 1, fileSize, file);
  if (bytesRead < fileSize) {
    fprintf(stderr, "Could not read file \"%s\".\n", path);
    exit(74);
  }
  utf8Buffer[bytesRead] = '\0';

  fclose(file);

  size_t wideSize = mbstowcs(NULL, utf8Buffer, 0) + 1;
  wchar_t *wideBuffer = (wchar_t*)malloc(wideSize * sizeof(wchar_t));
  if (wideBuffer == NULL) {
    fprintf(stderr, "Not enough memory to convert \"%s\".\n", path);
    exit(74);
  }

  mbstowcs(wideBuffer, utf8Buffer, wideSize);
  free(utf8Buffer);

  return wideBuffer;
}

static void runFile(const char *path) {
  wchar_t *source = readFile(path);
  InterpretResult result = interpret(source);
  free(source);

  if (result == INTERPRET_COMPILE_ERROR) exit(65);
  if (result == INTERPRET_RUNTIME_ERROR) exit(60);
}
/**
 * main - entry point for ahadu.
 * @argc: argument count
 * @argv: the arguments
 * Return: 0
 */
int main(int argc, const char *argv[])
{
  setlocale(LC_ALL, "");
  initVM();

  if (argc == 1) {
    repl();
  } else if (argc == 2) {
    runFile(argv[1]);
  } else {
    fprintf(stderr, "Usage: ahadu [path]\n");
    exit(64);
  }

  freeVM();
  return 0;
}
