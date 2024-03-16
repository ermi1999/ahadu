#include <stdio.h>
#include <stdlib.h>

#include "common.h"
#include "compiler.h"
#include "scanner.h"

#ifdef DEBUG_PRINT_CODE
  #include "debug.h"
#endif

typedef struct {
  Token current;
  Token previous;
  bool hadError;
  bool panicMode;
} Parser;

/**
 * Presedence - the precedence levels for the parser.
 * PREC_NONE: the lowest precedence level.
 * PREC_PRIMARY: the highest precedence level.
 * @note: the precedence levels are used to determine the order of operations for the parser. and they are listed from the lowest to the highest precedence level. it is because C implicitly assigns the lowest value to the first element in an enum and then increments the value for each subsequent element.
 */
typedef enum {
  PREC_NONE,
  PREC_ASSIGNMENT,  // =
  PREC_OR,          // or
  PREC_AND,         // and
  PREC_EQUALITY,    // == !=
  PREC_COMPARISON,  // < > <= >=
  PREC_TERM,        // + -
  PREC_FACTOR,      // * /
  PREC_UNARY,       // ! -
  PREC_CALL,        // . ()
  PREC_PRIMARY
} Precedence;

typedef void (*ParseFn)();

typedef struct {
  ParseFn prefix;
  ParseFn infix;
  Precedence precedence;
} ParseRule;

Parser parser;
Chunk *compilingChunk;

/**
 * currentChunk - returns currently compiling chunk.
 */
static Chunk *currentChunk() {
  return compilingChunk;
}

/**
 * errorAt - handles a syntax error and updates hadError flag to true.
 */
static void errorAt(Token *token, const wchar_t *message) {
  if (parser.panicMode) return;
  parser.panicMode = true;
  fprintf(stderr, "[line %d] Error", token->line);

  if (token->type == TOKEN_EOF) {
      fprintf(stderr, " at end");
  } else if (token->type == TOKEN_ERROR) {
    // Nothing.
  } else {
    fprintf(stderr, " at '%.*s'", token->length, token->start);
  }

  fprintf(stderr, ": %s\n", message);
  parser.hadError = true;
}

/**
 * error - redirects a syntax error to error at.
 */
static void error(const wchar_t *message) {
  errorAt(&parser.previous, message);
}

/**
 * errorAtCurrent - redirects a syntax error if the scanner returns an error token.
 */
static void errorAtCurrent(const wchar_t *message) {
  errorAt(&parser.current, message);
}

/**
  * advance - advances the parser to the next token.
  */
static void advance() {
  parser.previous = parser.current;

  for (;;) {
    parser.current = scanToken();
    if (parser.current.type != TOKEN_ERROR) break;

    errorAtCurrent(parser.current.start);
  }
}

/**
* consume - validates is a token is an exepected type and if it is it bumps the token to the next token.
* @type: the type to check for.
*/
static void consume(TokenType type, const wchar_t *message) {
  if (parser.current.type == type) {
    advance();
    return;
  }

  errorAtCurrent(message);
}

/**
  * emitByte - appends (writes) a single byte to chunk.
  * @byte: the byte to append (write)
  */
static void emitByte(uint8_t byte) {
  writeChunk(currentChunk(), byte, parser.previous.line);
}

/**
  * emitBytes - appends (writes) two bytes to chunk.
  * @byte1: the first byte to append (write)
  * @byte2: the second byte to append (write)
  */
static void emitBytes(uint8_t byte1, uint8_t byte2) {
  emitByte(byte1);
  emitByte(byte2);
}

/**
  * emitReturn - emits the return instruction.
  */
static void emitReturn() {
  emitByte(OP_RETURN);
}

/**
  * addConstant - adds a constant to the chunk.
  * @chunk: the chunk to add the constant to.
  * @value: the value to add.
  * @return: the index of the constant.
  * @TODO: OP_CONSTANT uses a single byte to store the index of the constant, so we can only have 256 constants in a single chunk. add another instruction that stores the index in two bytes.
  */
static uint8_t makeConstant(Value value) {
  int constant = addConstant(currentChunk(), value);
  if (constant > UINT8_MAX) {
    error(L"Too many constants in one chunk.");
    return 0;
  }

  return (uint8_t)constant;
}

/**
  * emitConstant - emits a constant instruction.
  * @value: the value to emit.
  */
static void emitConstant(double value) {
  emitBytes(OP_CONSTANT, makeConstant(value));
}

/**
 * endCompiler - ends the compiler and emits the return instruction.
 */
static void endCompiler() {
  emitReturn();
  #ifdef DEBUG_PRINT_CODE
    if (!parser.hadError) {
      disassembleChunk(currentChunk(), "code");
    }
  #endif
}

/**
 * number - compiles a number constant.
 */
static void number() {
  double value = wcstod(parser.previous.start, NULL);
  emitConstant(value);
}

static void expression();
static ParseRule *getRule(TokenType type);
static void parsePrecedence(Precedence precedence);

static void binary() {
  // Remember the operator.
  TokenType operatorType = parser.previous.type;

  // Compile the right operand.
  ParseRule *rule = getRule(operatorType);
  parsePrecedence((Precedence)(rule->precedence + 1));

  // Emit the operator instruction.
  switch (operatorType) {
    case TOKEN_PLUS: emitByte(OP_ADD); break;
    case TOKEN_MINUS: emitByte(OP_SUBTRACT); break;
    case TOKEN_STAR: emitByte(OP_MULTIPLY); break;
    case TOKEN_SLASH: emitByte(OP_DIVIDE); break;
    default: return; // Unreachable.
  }
}

/**
 * grouping - compiles a grouping / () expression.
 */
static void grouping() {
  expression();
  consume(TOKEN_RIGHT_PAREN, L"Expect ')' after expression.");
}

/**
 * unary - compiles a unary (-, !) expression.
 */
static void unary() {
  TokenType operatorType = parser.previous.type;

  // Compile the operand.
  parsePrecedence(PREC_UNARY);

  // Emit the operator instruction.
  switch (operatorType) {
    case TOKEN_MINUS: emitByte(OP_NEGATE); break;
    default: return; // Unreachable.
  }
}

/**
 * rules - the parse rules for the compiler.
 * @note: the parse rules are used to determine the precedence and associativity of the operators in the language.
 
 */
ParseRule rules[] = {
  [TOKEN_LEFT_PAREN]    = {grouping, NULL,   PREC_NONE},
  [TOKEN_RIGHT_PAREN]   = {NULL,     NULL,   PREC_NONE},
  [TOKEN_LEFT_BRACE]    = {NULL,     NULL,   PREC_NONE},
  [TOKEN_RIGHT_BRACE]   = {NULL,     NULL,   PREC_NONE},
  [TOKEN_COMMA]         = {NULL,     NULL,   PREC_NONE},
  [TOKEN_DOT]           = {NULL,     NULL,   PREC_NONE},
  [TOKEN_MINUS]         = {unary,    binary, PREC_TERM},
  [TOKEN_PLUS]          = {NULL,     binary, PREC_TERM},
  [TOKEN_SEMICOLON]     = {NULL,     NULL,   PREC_NONE},
  [TOKEN_SLASH]         = {NULL,     binary, PREC_FACTOR},
  [TOKEN_STAR]          = {NULL,     binary, PREC_FACTOR},
  [TOKEN_BANG]          = {NULL,     NULL,   PREC_NONE},
  [TOKEN_BANG_EQUAL]    = {NULL,     NULL,   PREC_NONE},
  [TOKEN_EQUAL]         = {NULL,     NULL,   PREC_NONE},
  [TOKEN_EQUAL_EQUAL]   = {NULL,     NULL,   PREC_NONE},
  [TOKEN_GREATER]       = {NULL,     NULL,   PREC_NONE},
  [TOKEN_GREATER_EQUAL] = {NULL,     NULL,   PREC_NONE},
  [TOKEN_LESS]          = {NULL,     NULL,   PREC_NONE},
  [TOKEN_LESS_EQUAL]    = {NULL,     NULL,   PREC_NONE},
  [TOKEN_IDENTIFIER]    = {NULL,     NULL,   PREC_NONE},
  [TOKEN_STRING]        = {NULL,     NULL,   PREC_NONE},
  [TOKEN_NUMBER]        = {number,   NULL,   PREC_NONE},
  [TOKEN_AND]           = {NULL,     NULL,   PREC_NONE},
  [TOKEN_CLASS]         = {NULL,     NULL,   PREC_NONE},
  [TOKEN_ELSE]          = {NULL,     NULL,   PREC_NONE},
  [TOKEN_FALSE]         = {NULL,     NULL,   PREC_NONE},
  [TOKEN_FOR]           = {NULL,     NULL,   PREC_NONE},
  [TOKEN_FUN]           = {NULL,     NULL,   PREC_NONE},
  [TOKEN_IF]            = {NULL,     NULL,   PREC_NONE},
  [TOKEN_NIL]           = {NULL,     NULL,   PREC_NONE},
  [TOKEN_OR]            = {NULL,     NULL,   PREC_NONE},
  [TOKEN_PRINT]         = {NULL,     NULL,   PREC_NONE},
  [TOKEN_RETURN]        = {NULL,     NULL,   PREC_NONE},
  [TOKEN_SUPER]         = {NULL,     NULL,   PREC_NONE},
  [TOKEN_THIS]          = {NULL,     NULL,   PREC_NONE},
  [TOKEN_TRUE]          = {NULL,     NULL,   PREC_NONE},
  [TOKEN_VAR]           = {NULL,     NULL,   PREC_NONE},
  [TOKEN_WHILE]         = {NULL,     NULL,   PREC_NONE},
  [TOKEN_ERROR]         = {NULL,     NULL,   PREC_NONE},
  [TOKEN_EOF]           = {NULL,     NULL,   PREC_NONE},
  [TOKEN_ERROR]         = {NULL,     NULL,   PREC_NONE},
  [TOKEN_EOF]           = {NULL,     NULL,   PREC_NONE},
};

/**
 * parsePrecedence - parses a precedence level.
 * @precedence: the precedence level to parse.
 */
static void parsePrecedence(Precedence precedence) {
  advance();
  ParseFn prefixRule = getRule(parser.previous.type)->prefix;
  if (prefixRule == NULL) {
    error(L"Expect expression.");
    return;
  }

  prefixRule();

  while (precedence <= getRule(parser.current.type)->precedence) {
    advance();
    ParseFn infixRule = getRule(parser.previous.type)->infix;
    infixRule();
  }
}

static ParseRule *getRule(TokenType type) {
  return &rules[type];
}

/**
 * expression - compiles an expression.
 */
static void expression(){
  parsePrecedence(PREC_ASSIGNMENT);
}

/**
  * initCompiler - initializes the compiler.
  * @source: the source code to compile.
  * @chunk: the chunk to initialize.
  * @return: true if the source code was compiled successfully, false otherwise.
  */
bool compile(const wchar_t *source, Chunk *chunk) {
  initScanner(source);
  compilingChunk = chunk;
  
  parser.hadError = false;
  parser.panicMode = false;

  advance();
  expression();
  consume(TOKEN_EOF, L"Expect end of expression.");
  endCompiler();
  return !parser.hadError;
}
