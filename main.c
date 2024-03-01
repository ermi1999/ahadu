#include "common.h"
#include "chunk.h"
#include "debug.h"

/**
 * main - entry point for ahadu.
 * @argc: argument count
 * @argv: the arguments
 * Return: 0
 */
int main(int argc, const char *argv[])
{
  Chunk chunk;
  initChunk(&chunk);
  writeChunk(&chunk, OP_RETURN);
  disassembleChunk(&chunk, "test chunk");
  freeChunk(&chunk);
  return 0;
}
