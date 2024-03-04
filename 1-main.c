#include "common.h"
#include "chunk.h"
#include "debug.h"
#include "vm.h"

/**
 * main - entry point for ahadu.
 * @argc: argument count
 * @argv: the arguments
 * Return: 0
 */
int main(int argc, const char *argv[])
{
    initVM();
    
    Chunk chunk;
    initChunk(&chunk);
    
    int constant = addConstant(&chunk, 4);
    writeChunk(&chunk, OP_CONSTANT, 123);
    writeChunk(&chunk, constant, 123);

    constant = addConstant(&chunk, 3);
    writeChunk(&chunk, OP_CONSTANT, 123);
    writeChunk(&chunk, constant, 123);

    writeChunk(&chunk, OP_SUBTRACT, 123);

    constant = addConstant(&chunk, 2);
    writeChunk(&chunk, OP_CONSTANT, 123);
    writeChunk(&chunk, constant, 123);

    writeChunk(&chunk, OP_NEGATE, 123);

    writeChunk(&chunk, OP_MULTIPLY, 123);
    writeChunk(&chunk, OP_RETURN, 123);
    disassembleChunk(&chunk, "test chunk");
    interpret(&chunk);
    freeVM();
    freeChunk(&chunk);
    return 0;
}