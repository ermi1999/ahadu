#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "common.h"
#include "compiler.h"
#include "memory.h"
#include "scanner.h"
#include "object.h"

#ifdef DEBUG_PRINT_CODE
#include "debug.h"
#endif

typedef struct
{
  Token current;
  Token previous;
  bool hadError;
  bool panicMode;
} Parser;

/**
 * Presedence - the precedence levels for the parser.
 * PREC_NONE: the lowest precedence level.
 * PREC_PRIMARY: the highest precedence level.
 * @note: the precedence levels are used to determine the order of operations for the parser.
 *        and they are listed from the lowest to the highest precedence level.
 *        it is because C implicitly assigns the lowest value to the first element in an enum
 *        and then increments the value for each subsequent element.
 */
typedef enum
{
  PREC_NONE,
  PREC_ASSIGNMENT, // =
  PREC_OR,         // or
  PREC_AND,        // and
  PREC_EQUALITY,   // == !=
  PREC_COMPARISON, // < > <= >=
  PREC_TERM,       // + -
  PREC_FACTOR,     // * /
  PREC_UNARY,      // ! -
  PREC_CALL,       // . ()
  PREC_PRIMARY
} Precedence;

typedef void (*ParseFn)(bool canAssign);

typedef struct
{
  ParseFn prefix;
  ParseFn infix;
  Precedence precedence;
} ParseRule;

typedef struct
{
  Token name;
  int depth;
  bool isCaptured;
} Local;

typedef struct
{
  uint8_t index;
  bool isLocal;
} Upvalue;

typedef enum
{
  TYPE_FUNCTION,
  TYPE_INITIALIZER,
  TYPE_METHOD,
  TYPE_SCRIPT
} FunctionType;

typedef struct Compiler
{
  struct Compiler *enclosing;
  ObjFunction *function;
  FunctionType type;

  Local locals[UINT8_COUNT];
  int localCount;
  Upvalue upvalues[UINT8_COUNT];
  int scopeDepth;
} Compiler;

typedef struct ClassCompiler
{
  struct ClassCompiler *enclosing;
  bool hasSuperclass;
} ClassCompiler;

Parser parser;
Compiler *current = NULL;
Chunk *compilingChunk;
ClassCompiler *currentClass = NULL;

/**
 * currentChunk - returns currently compiling chunk.
 */
static Chunk *currentChunk()
{
  return &current->function->chunk;
}

/**
 * errorAt - handles a syntax error and updates hadError flag to true.
 */
static void errorAt(Token *token, const wchar_t *message)
{
  if (parser.panicMode)
    return;
  parser.panicMode = true;
  fwprintf(stderr, L"[መስመር %d] ላይ ስህተት", token->line);

  if (token->type == TOKEN_EOF)
  {
    fwprintf(stderr, L" መጨረሻ ላይ");
  }
  else if (token->type == TOKEN_ERROR)
  {
    // Nothing.
  }
  else
  {
    fwprintf(stderr, L" '%.*s' ጋ", token->length, token->start);
  }

  fwprintf(stderr, L": %s\n", message);
  parser.hadError = true;
}

/**
 * error - redirects a syntax error to error at.
 */
static void error(const wchar_t *message)
{
  errorAt(&parser.previous, message);
}

/**
 * errorAtCurrent - redirects a syntax error if the scanner returns an error token.
 */
static void errorAtCurrent(const wchar_t *message)
{
  errorAt(&parser.current, message);
}

/**
 * advance - advances the parser to the next token.
 */
static void advance()
{
  parser.previous = parser.current;

  for (;;)
  {
    parser.current = scanToken();
    if (parser.current.type != TOKEN_ERROR)
      break;

    errorAtCurrent(parser.current.start);
  }
}

/**
 * consume - validates is a token is an exepected type and if it is it bumps the token to the next token.
 * @type: the type to check for.
 */
static void consume(TokenType type, const wchar_t *message)
{
  if (parser.current.type == type)
  {
    advance();
    return;
  }

  errorAtCurrent(message);
}

static bool check(TokenType type)
{
  return parser.current.type == type;
}

static bool match(TokenType type)
{
  if (!check(type))
    return false;
  advance();
  return true;
}

/**
 * emitByte - appends (writes) a single byte to chunk.
 * @byte: the byte to append (write)
 */
static void emitByte(uint8_t byte)
{
  writeChunk(currentChunk(), byte, parser.previous.line);
}

/**
 * emitBytes - appends (writes) two bytes to chunk.
 * @byte1: the first byte to append (write)
 * @byte2: the second byte to append (write)
 */
static void emitBytes(uint8_t byte1, uint8_t byte2)
{
  emitByte(byte1);
  emitByte(byte2);
}

/**
 * emitLoop - emits a loop instruction.
 * @loopStart: the start of the loop.
 */
static void emitLoop(int loopStart)
{
  emitByte(OP_LOOP);

  int offset = currentChunk()->count - loopStart + 2;
  if (offset > UINT16_MAX)
  {
    error(L"ተመላላሹ በጣም ትልቅ ነው።");
  }

  emitByte((offset >> 8) & 0xff);
  emitByte(offset & 0xff);
}

static int emitJump(uint8_t instruction)
{
  emitByte(instruction);
  emitByte(0xff);
  emitByte(0xff);
  return currentChunk()->count - 2;
}

/**
 * emitReturn - emits the return instruction.
 */
static void emitReturn()
{
  if (current->type == TYPE_INITIALIZER)
  {
    emitBytes(OP_GET_LOCAL, 0);
  }
  else
  {
    emitByte(OP_NIL);
  }
  emitByte(OP_RETURN);
}

/**
 * addConstant - adds a constant to the chunk.
 * @chunk: the chunk to add the constant to.
 * @value: the value to add.
 * @return: the index of the constant.
 * @TODO: OP_CONSTANT uses a single byte to store the index of the constant, so we can only have 256 constants in a single chunk. add another instruction that stores the index in two bytes.
 */
static uint8_t makeConstant(Value value)
{
  int constant = addConstant(currentChunk(), value);
  if (constant > UINT8_MAX)
  {
    error(L"በ አንድ ቸንክ ውስጥ ብዙ መረጃዎች።");
    return 0;
  }

  return (uint8_t)constant;
}

/**
 * emitConstant - emits a constant instruction.
 * @value: the value to emit.
 */
static void emitConstant(Value value)
{
  emitBytes(OP_CONSTANT, makeConstant(value));
}

static void patchJump(int offset)
{
  // -2 to adjust for the bytecode for the jump offset itself.
  int jump = currentChunk()->count - offset - 2;

  if (jump > UINT16_MAX)
  {
    error(L"ኮዱን ለመዝለል አስቸጋሪ ነው።");
  }

  currentChunk()->code[offset] = (jump >> 8) & 0xff;
  currentChunk()->code[offset + 1] = jump & 0xff;
}

/**
 * initCompiler - initializes the compiler.
 * @compiler: the compiler to initialize.
 */
static void initCompiler(Compiler *compiler, FunctionType type)
{
  compiler->enclosing = current;
  compiler->function = NULL;
  compiler->type = type;
  compiler->localCount = 0;
  compiler->function = newFunction();
  current = compiler;

  if (type != TYPE_SCRIPT)
  {
    current->function->name = copyString(parser.previous.start, parser.previous.length);
  }

  Local *local = &current->locals[current->localCount++];
  local->depth = 0;
  local->isCaptured = false;
  if (type != TYPE_FUNCTION)
  {
    local->name.start = L"ይህ";
    local->name.length = 2;
  }
  else
  {
    local->name.start = L"";
    local->name.length = 0;
  }
}

/**
 * endCompiler - ends the compiler and emits the return instruction.
 */
static ObjFunction *endCompiler()
{
  emitReturn();
  ObjFunction *function = current->function;
#ifdef DEBUG_PRINT_CODE
  if (!parser.hadError)
  {
    disassembleChunk(currentChunk(), function->name != NULL ? (char *)function->name->chars : "<script>");
  }
#endif

  current = current->enclosing;
  return function;
}

/**
 * beginScope - begins a new scope.
 */
static void beginScope()
{
  current->scopeDepth++;
}

/**
 * endScope - ends the current scope.
 */
static void endScope()
{
  current->scopeDepth--;

  while (current->localCount > 0 && current->locals[current->localCount - 1].depth > current->scopeDepth)
  {
    if (current->locals[current->localCount - 1].isCaptured)
    {
      emitByte(OP_CLOSE_UPVALUE);
    }
    else
    {
      emitByte(OP_POP);
    }
    current->localCount--;
  }
}

/**
 * identifierConstant - returns the index of a constant.
 * @name: the name of the constant.
 * @return: the index of the constant.
 */
static uint8_t identifierConstant(Token *name)
{
  return makeConstant(OBJ_VAL(copyString(name->start, name->length)));
}

/**
 * identifiersEqual - checks if two identifiers are equal.
 * @a: the first identifier.
 * @b: the second identifier.
 * @return: true if the identifiers are equal, false otherwise.
 */
static bool identifiersEqual(Token *a, Token *b)
{
  if (a->length != b->length)
    return false;
  return memcmp(a->start, b->start, a->length) == 0;
}

static int resolveLocal(Compiler *compiler, Token *name)
{
  for (int i = compiler->localCount - 1; i >= 0; i--)
  {
    Local *local = &compiler->locals[i];
    if (identifiersEqual(name, &local->name))
    {
      if (local->depth == -1)
      {
        error(L"የ ክልል መለያን የሚሰየምበት ቦታ ላይ ማንበብ አይቻልም።");
      }
      return i;
    }
  }

  return -1;
}

/**
 * addUpvalue - adds an upvalue.
 * @compiler: the compiler to add the upvalue to.
 * @index: the index of the upvalue.
 * @isLocal: true if the upvalue is local, false otherwise.
 * @return: the index of the upvalue.
 */
static int addUpvalue(Compiler *compiler, uint8_t index, bool isLocal)
{
  int upvalueCount = compiler->function->upvalueCount;

  for (int i = 0; i < upvalueCount; i++)
  {
    Upvalue *upvalue = &compiler->upvalues[i];
    if (upvalue->index == index && upvalue->isLocal == isLocal)
    {
      return i;
    }
  }

  if (upvalueCount == UINT8_COUNT)
  {
    error(L"Too many closure variables in function.");
    return 0;
  }

  compiler->upvalues[upvalueCount].isLocal = isLocal;
  compiler->upvalues[upvalueCount].index = index;
  return compiler->function->upvalueCount++;
}

static int resolveUpvalue(Compiler *compiler, Token *name)
{
  if (compiler->enclosing == NULL)
    return -1;

  int local = resolveLocal(compiler->enclosing, name);
  if (local != -1)
  {
    compiler->enclosing->locals[local].isCaptured = true;
    return addUpvalue(compiler, (uint8_t)local, true);
  }

  int upvalue = resolveUpvalue(compiler->enclosing, name);
  if (upvalue != -1)
  {
    return addUpvalue(compiler, (uint8_t)upvalue, false);
  }

  return -1;
}

/**
 * addLocal - adds a local variable.
 * @name: the name of the variable.
 */
static void addLocal(Token name)
{
  if (current->localCount == UINT8_COUNT)
  {
    error(L"በ አንድ ተግባር ውስጥ ብዙ የክልል መለያዎች።");
    return;
  }

  Local *local = &current->locals[current->localCount++];
  local->name = name;
  local->depth = -1;
  local->isCaptured = false;
}

/**
 * declareVariable - declares a variable.
 */
static void declareVariable()
{
  if (current->scopeDepth == 0)
    return;

  Token *name = &parser.previous;

  for (int i = current->localCount - 1; i >= 0; i--)
  {
    Local *local = &current->locals[i];
    if (local->depth != -1 && local->depth < current->scopeDepth)
    {
      break;
    }

    if (identifiersEqual(name, &local->name))
    {
      error(L"ተመሳሳይ የመለያ ስም በእይታ ውስጣ አለ።");
    }
  }

  addLocal(*name);
}

/**
 * number - compiles a number constant.
 */
static void number(bool canAssign)
{
  double value = wcstod(parser.previous.start, NULL);
  emitConstant(NUMBER_VAL(value));
}

/**
 * string - compiles a string constant.
 */
static void string(bool canAssign)
{
  emitConstant(OBJ_VAL(copyString(parser.previous.start + 1, parser.previous.length - 2)));
}

static void expression();
static void statement();
static void declaration();
static ParseRule *getRule(TokenType type);
static void parsePrecedence(Precedence precedence);

/**
 * or_ - compiles an or expression.
 * @canAssign: true if the expression can be assigned, false otherwise.
 */
static void or_(bool canAssign)
{
  int elseJump = emitJump(OP_JUMP_IF_FALSE);
  int endJump = emitJump(OP_JUMP);

  patchJump(elseJump);
  emitByte(OP_POP);

  parsePrecedence(PREC_OR);
  patchJump(endJump);
}

/**
 * namedVariable - compiles a named variable.
 * @name: the name of the variable.
 */
static void namedVariable(Token name, bool canAssign)
{
  uint8_t getOp, setOp;
  int arg = resolveLocal(current, &name);
  if (arg != -1)
  {
    getOp = OP_GET_LOCAL;
    setOp = OP_SET_LOCAL;
  }
  else if ((arg = resolveUpvalue(current, &name)) != -1)
  {
    getOp = OP_GET_UPVALUE;
    setOp = OP_SET_UPVALUE;
  }
  else
  {
    arg = identifierConstant(&name);
    getOp = OP_GET_GLOBAL;
    setOp = OP_SET_GLOBAL;
  }

  if (canAssign && match(TOKEN_EQUAL))
  {
    expression();
    emitBytes(setOp, (uint8_t)arg);
  }
  else
  {
    emitBytes(getOp, (uint8_t)arg);
  }
}

/**
 * variable - compiles a variable expression.
 */
static void variable(bool canAssign)
{
  namedVariable(parser.previous, canAssign);
}

/**
 * argumentList - compiles an argument list.
 * @return: the number of arguments.
 */
static uint8_t argumentList()
{
  uint8_t argCount = 0;
  if (!check(TOKEN_RIGHT_PAREN))
  {
    do
    {
      expression();
      if (argCount == 255)
      {
        error(L"ከ 255 በላይ የ ተግባር መለኪያዎችን መስጠት አይቻልም።");
      }
      argCount++;
    } while (match(TOKEN_COMMA));
  }

  consume(TOKEN_RIGHT_PAREN, L"ከ ተግባር መለኪያዎች በሗላ ')' ያስፈልጋል።");
  return argCount;
}

/**
 * syntheticToken - creates a synthetic token.
 * @text: the text of the token.
 * @return: the synthetic token.
 */
static Token syntheticToken(const wchar_t *text)
{
  Token token;
  token.start = text;
  token.length = (int)wcslen(text);
  return token;
}

/**
 * super_ - compiles a super expression.
 */
static void super_(bool canAssign)
{
  if (currentClass == NULL)
  {
    error(L"ታላቅን ከ ክፍል ውጪ መጠቀም አይቻልም።");
  }
  else if (!currentClass->hasSuperclass)
  {
    error(L"ታላቅን ታላቅ ክፍል የሌለው ክፍል ውስጥ መጠቀም አይቻልም።");
  }

  consume(TOKEN_DOT, L"ከ 'ታላቅ' በሗላ '.' ያስፈልጋል።");
  consume(TOKEN_IDENTIFIER, L"የ 'ታላቅ' ክፍል ተግባር ስም ያስፈልጋል።");
  uint8_t name = identifierConstant(&parser.previous);

  namedVariable(syntheticToken(L"ይህ"), false);

  if (match(TOKEN_LEFT_PAREN))
  {
    uint8_t argCount = argumentList();
    namedVariable(syntheticToken(L"ታላቅ"), false);
    emitBytes(OP_SUPER_INVOKE, name);
    emitByte(argCount);
  }
  else
  {
    namedVariable(syntheticToken(L"ታላቅ"), false);
    emitBytes(OP_GET_SUPER, name);
  }
}

/**
 * this_ - compiles a this expression.
 */
static void this_(bool canAssign)
{
  if (currentClass == NULL)
  {
    error(L"ከ ክፍል ውጪ 'ይህ'ን መጠቀም አይቻልም።");
    return;
  }
  variable(false);
}

static void binary(bool canAssign)
{
  // Remember the operator.
  TokenType operatorType = parser.previous.type;

  // Compile the right operand.
  ParseRule *rule = getRule(operatorType);
  parsePrecedence((Precedence)(rule->precedence + 1));

  // Emit the operator instruction.
  switch (operatorType)
  {
  case TOKEN_BANG_EQUAL:
    emitBytes(OP_EQUAL, OP_NOT);
    break;
  case TOKEN_EQUAL_EQUAL:
    emitByte(OP_EQUAL);
    break;
  case TOKEN_GREATER:
    emitByte(OP_GREATER);
    break;
  case TOKEN_GREATER_EQUAL:
    emitBytes(OP_LESS, OP_NOT);
    break;
  case TOKEN_LESS:
    emitByte(OP_LESS);
    break;
  case TOKEN_LESS_EQUAL:
    emitBytes(OP_GREATER, OP_NOT);
    break;
  case TOKEN_PLUS:
    emitByte(OP_ADD);
    break;
  case TOKEN_MINUS:
    emitByte(OP_SUBTRACT);
    break;
  case TOKEN_STAR:
    emitByte(OP_MULTIPLY);
    break;
  case TOKEN_SLASH:
    emitByte(OP_DIVIDE);
    break;
  default:
    return; // Unreachable.
  }
}

/**
 * call - compiles a call expression.
 * @canAssign: true if the expression can be assigned, false otherwise.
 */
static void call(bool canAssign)
{
  uint8_t argCount = argumentList();
  emitBytes(OP_CALL, argCount);
}

/**
 * dot - compiles a dot expression.
 * @canAssign: true if the expression can be assigned, false otherwise.
 */
static void dot(bool canAssign)
{
  consume(TOKEN_IDENTIFIER, L"ከ'.' በሗላ የንብረቱ ስም ያስፈልጋል።");
  uint8_t name = identifierConstant(&parser.previous);

  if (canAssign && match(TOKEN_EQUAL))
  {
    expression();
    emitBytes(OP_SET_PROPERTY, name);
  }
  else if (match(TOKEN_LEFT_PAREN))
  {
    uint8_t argCount = argumentList();
    emitBytes(OP_INVOKE, name);
    emitByte(argCount);
  }
  else
  {
    emitBytes(OP_GET_PROPERTY, name);
  }
}

static void literal(bool canAssign)
{
  switch (parser.previous.type)
  {
  case TOKEN_FALSE:
    emitByte(OP_FALSE);
    break;
  case TOKEN_NIL:
    emitByte(OP_NIL);
    break;
  case TOKEN_TRUE:
    emitByte(OP_TRUE);
    break;
  default:
    return; // Unreachable.
  }
}

/**
 * grouping - compiles a grouping / () expression.
 */
static void grouping(bool canAssign)
{
  expression();
  consume(TOKEN_RIGHT_PAREN, L"ከመግለፃው በሗላ ')' ያስፈልጋል።");
}

/**
 * unary - compiles a unary (-, !) expression.
 */
static void unary(bool canAssign)
{
  TokenType operatorType = parser.previous.type;

  // Compile the operand.
  parsePrecedence(PREC_UNARY);

  // Emit the operator instruction.
  switch (operatorType)
  {
  case TOKEN_BANG:
    emitByte(OP_NOT);
    break;
  case TOKEN_MINUS:
    emitByte(OP_NEGATE);
    break;
  default:
    return; // Unreachable.
  }
}

/**
 * and_ - compiles an and expression.
 * @canAssign: true if the expression can be assigned, false otherwise.

 */
static void and_(bool canAssign)
{
  int endJump = emitJump(OP_JUMP_IF_FALSE);

  emitByte(OP_POP);
  parsePrecedence(PREC_AND);

  patchJump(endJump);
}

/**
 * rules - the parse rules for the compiler.
 * @note: the parse rules are used to determine the precedence and associativity of the operators in the language.

 */
ParseRule rules[] = {
    [TOKEN_LEFT_PAREN] = {grouping, call, PREC_CALL},
    [TOKEN_RIGHT_PAREN] = {NULL, NULL, PREC_NONE},
    [TOKEN_LEFT_BRACE] = {NULL, NULL, PREC_NONE},
    [TOKEN_RIGHT_BRACE] = {NULL, NULL, PREC_NONE},
    [TOKEN_COMMA] = {NULL, NULL, PREC_NONE},
    [TOKEN_DOT] = {NULL, dot, PREC_CALL},
    [TOKEN_MINUS] = {unary, binary, PREC_TERM},
    [TOKEN_PLUS] = {NULL, binary, PREC_TERM},
    [TOKEN_SEMICOLON] = {NULL, NULL, PREC_NONE},
    [TOKEN_SLASH] = {NULL, binary, PREC_FACTOR},
    [TOKEN_STAR] = {NULL, binary, PREC_FACTOR},
    [TOKEN_BANG] = {unary, NULL, PREC_NONE},
    [TOKEN_BANG_EQUAL] = {NULL, binary, PREC_COMPARISON},
    [TOKEN_EQUAL] = {NULL, NULL, PREC_NONE},
    [TOKEN_EQUAL_EQUAL] = {NULL, binary, PREC_EQUALITY},
    [TOKEN_GREATER] = {NULL, binary, PREC_COMPARISON},
    [TOKEN_GREATER_EQUAL] = {NULL, binary, PREC_COMPARISON},
    [TOKEN_LESS] = {NULL, binary, PREC_COMPARISON},
    [TOKEN_LESS_EQUAL] = {NULL, binary, PREC_COMPARISON},
    [TOKEN_IDENTIFIER] = {variable, NULL, PREC_NONE},
    [TOKEN_STRING] = {string, NULL, PREC_NONE},
    [TOKEN_NUMBER] = {number, NULL, PREC_NONE},
    [TOKEN_AND] = {NULL, and_, PREC_AND},
    [TOKEN_CLASS] = {NULL, NULL, PREC_NONE},
    [TOKEN_ELSE] = {NULL, NULL, PREC_NONE},
    [TOKEN_FALSE] = {literal, NULL, PREC_NONE},
    [TOKEN_FOR] = {NULL, NULL, PREC_NONE},
    [TOKEN_FUN] = {NULL, NULL, PREC_NONE},
    [TOKEN_IF] = {NULL, NULL, PREC_NONE},
    [TOKEN_NIL] = {literal, NULL, PREC_NONE},
    [TOKEN_OR] = {NULL, or_, PREC_OR},
    [TOKEN_PRINT] = {NULL, NULL, PREC_NONE},
    [TOKEN_RETURN] = {NULL, NULL, PREC_NONE},
    [TOKEN_SUPER] = {super_, NULL, PREC_NONE},
    [TOKEN_THIS] = {this_, NULL, PREC_NONE},
    [TOKEN_TRUE] = {literal, NULL, PREC_NONE},
    [TOKEN_VAR] = {NULL, NULL, PREC_NONE},
    [TOKEN_WHILE] = {NULL, NULL, PREC_NONE},
    [TOKEN_ERROR] = {NULL, NULL, PREC_NONE},
    [TOKEN_EOF] = {NULL, NULL, PREC_NONE},
    [TOKEN_ERROR] = {NULL, NULL, PREC_NONE},
    [TOKEN_EOF] = {NULL, NULL, PREC_NONE},
};

/**
 * parsePrecedence - parses a precedence level.
 * @precedence: the precedence level to parse.
 */
static void parsePrecedence(Precedence precedence)
{
  advance();
  ParseFn prefixRule = getRule(parser.previous.type)->prefix;
  if (prefixRule == NULL)
  {
    error(L"አገላለጽ ያስፈልጋል።");
    return;
  }

  bool canAssign = precedence <= PREC_ASSIGNMENT;
  prefixRule(canAssign);

  while (precedence <= getRule(parser.current.type)->precedence)
  {
    advance();
    ParseFn infixRule = getRule(parser.previous.type)->infix;
    infixRule(canAssign);
  }

  if (canAssign && match(TOKEN_EQUAL))
  {
    error(L"ልክ ያልሆነ ኢላማ ላይ ነው ለመመደብ የሞከርከው።");
  }
}

/**
 * parseVariable - parses a variable.
 * @errorMessage: the error message to display if the variable is not found.
 * @return: the index of the variable.
 */
static uint8_t parseVariable(const wchar_t *errorMessage)
{
  consume(TOKEN_IDENTIFIER, errorMessage);
  declareVariable();
  if (current->scopeDepth > 0)
    return 0;
  return identifierConstant(&parser.previous);
}

/**
 * markInitialized - marks a variable as initialized.
 */
static void markInitialized()
{
  if (current->scopeDepth == 0)
    return;
  current->locals[current->localCount - 1].depth = current->scopeDepth;
}

/**
 * defineVariable - defines a variable.
 * @global: the index of the variable.
 */
static void defineVariable(uint8_t global)
{
  if (current->scopeDepth > 0)
  {
    markInitialized();
    return;
  }
  emitBytes(OP_DEFINE_GLOBAL, global);
}

static ParseRule *getRule(TokenType type)
{
  return &rules[type];
}

/**
 * expression - compiles an expression.
 */
static void expression()
{
  parsePrecedence(PREC_ASSIGNMENT);
}

/**
 * block - compiles a block.
 */
static void block()
{
  while (!check(TOKEN_RIGHT_BRACE) && !check(TOKEN_EOF))
  {
    declaration();
  }

  consume(TOKEN_RIGHT_BRACE, L"ከአጥር በሗላ '}' ያስፈልጋል።");
}

/**
 * function - compiles a function.
 * @type: the type of the function.
 */
static void function(FunctionType type)
{
  Compiler compiler;
  initCompiler(&compiler, type);
  beginScope();

  consume(TOKEN_LEFT_PAREN, L"ከ ተግባር ስም በሗላ '(' ያስፈልጋል።");

  if (!check(TOKEN_RIGHT_PAREN))
  {
    do
    {
      current->function->arity++;
      if (current->function->arity > 255)
      {
        errorAtCurrent(L"ከ 255 በላይ መለኪያዎችን መጠቀም አይቻልም።");
      }

      uint8_t constant = parseVariable(L"የመለኪያ ስም ያስፈልጋል።");
      defineVariable(constant);
    } while (match(TOKEN_COMMA));
  }

  consume(TOKEN_RIGHT_PAREN, L"ከተግባር መለኪያዎች በሗላ ')' ያስፈልጋል።");
  consume(TOKEN_LEFT_BRACE, L"ከተግባር አካል በፊት '{' ያስፈልጋል።");
  block();

  ObjFunction *function = endCompiler();
  emitBytes(OP_CLOSURE, makeConstant(OBJ_VAL(function)));

  for (int i = 0; i < function->upvalueCount; i++)
  {
    emitByte(compiler.upvalues[i].isLocal ? 1 : 0);
    emitByte(compiler.upvalues[i].index);
  }
}

static void method()
{
  consume(TOKEN_IDENTIFIER, L"የ ክፍል ተግባር ስም ያስፈልጋል።");
  uint8_t constant = identifierConstant(&parser.previous);

  FunctionType type = TYPE_METHOD;
  if (parser.previous.length == 6 && memcmp(parser.previous.start, L"ማስጀመሪያ", 6) == 0)
  {
    type = TYPE_INITIALIZER;
  }
  function(type);
  emitBytes(OP_METHOD, constant);
}

static void classDeclaration()
{
  consume(TOKEN_IDENTIFIER, L"የ ክፍል ስም ያስፈልጋል።");
  Token className = parser.previous;
  uint8_t nameConstant = identifierConstant(&parser.previous);
  declareVariable();

  emitBytes(OP_CLASS, nameConstant);
  defineVariable(nameConstant);

  ClassCompiler classCompiler;
  classCompiler.hasSuperclass = false;
  classCompiler.enclosing = currentClass;
  currentClass = &classCompiler;

  if (match(TOKEN_LESS))
  {
    consume(TOKEN_IDENTIFIER, L"የታላቅ ክፍል ስም ያስፈልጋል።");
    variable(false);

    if (identifiersEqual(&className, &parser.previous))
    {
      error(L"ክፍል እራሱን ሊወርስ አይችልም።");
    }

    beginScope();
    addLocal(syntheticToken(L"ታላቅ"));
    defineVariable(0);

    namedVariable(className, false);
    emitByte(OP_INHERIT);
    classCompiler.hasSuperclass = true;
  }
  namedVariable(className, false);
  consume(TOKEN_LEFT_BRACE, L"ከክፍል አካል በፊት '{' ያስፈልጋል።");
  while (!check(TOKEN_RIGHT_BRACE) && !check(TOKEN_EOF))
  {
    method();
  }
  consume(TOKEN_RIGHT_BRACE, L"ከክፍል አካል በሗላ '}' ያስፈልጋል።");
  emitByte(OP_POP);

  if (classCompiler.hasSuperclass)
  {
    endScope();
  }

  currentClass = currentClass->enclosing;
}

/**
 * funDeclaration - compiles a function declaration.
 */
static void funDeclaration()
{
  uint8_t global = parseVariable(L"የተግባር ስም ያስፈልጋል።");
  markInitialized();
  function(TYPE_FUNCTION);
  defineVariable(global);
}

/**
 * varDeclaration - compiles a variable declaration.
 */
static void varDeclaration()
{
  uint8_t global = parseVariable(L"የመለያ ስም ያስፈልጋል።");

  if (match(TOKEN_EQUAL))
  {
    expression();
  }
  else
  {
    emitByte(OP_NIL);
  }

  consume(TOKEN_SEMICOLON, L"ከ መለያ ገለጻ በሗላ ';' ያስፈልጋል።");

  defineVariable(global);
}

/**
 * expressionStatement - compiles an expression statement.
 */
static void expressionStatement()
{
  expression();
  consume(TOKEN_SEMICOLON, L"ከመግለጻ በሗላ ';' ያስፈልጋል።");
  emitByte(OP_POP);
}

/**
 * forStatement - compiles a for statement.
 */
static void forStatement()
{
  beginScope();
  consume(TOKEN_LEFT_PAREN, L"ከ'ለዚህ' በሗላ '(' ያስፈልጋል።");
  if (match(TOKEN_SEMICOLON))
  {
    // No initializer.
  }
  else if (match(TOKEN_VAR))
  {
    varDeclaration();
  }
  else
  {
    expressionStatement();
  }

  int loopStart = currentChunk()->count;
  int exitJump = -1;
  if (!match(TOKEN_SEMICOLON))
  {
    expression();
    consume(TOKEN_SEMICOLON, L"ከተደጋጋሚ 'ሁኔታ' በሗላ ';' ያስፈልጋል።");

    // Jump out of the loop if the condition is false.
    exitJump = emitJump(OP_JUMP_IF_FALSE);
    emitByte(OP_POP); // Condition.
  }

  if (!match(TOKEN_RIGHT_PAREN))
  {
    int bodyJump = emitJump(OP_JUMP);

    int incrementStart = currentChunk()->count;
    expression();
    emitByte(OP_POP);
    consume(TOKEN_RIGHT_PAREN, L"ከ'እስከ' አንቀጾች በሗላ ')' ያስፈልጋል");

    emitLoop(loopStart);
    loopStart = incrementStart;
    patchJump(bodyJump);
  }

  statement();
  emitLoop(loopStart);

  if (exitJump != -1)
  {
    patchJump(exitJump);
    emitByte(OP_POP); // Condition.
  }

  endScope();
}

/**
 * ifStatement - compiles an if statement.
 */
static void ifStatement()
{
  consume(TOKEN_LEFT_PAREN, L"ከ'ከሆነ' በሗላ '(' ያስፈልጋል።");
  expression();
  consume(TOKEN_RIGHT_PAREN, L"ከ'ሁኔታው' በሗላ ')' ያስፈልጋል።");

  // reserves space for a jump instruction.
  int thenJump = emitJump(OP_JUMP_IF_FALSE);
  // emits the condition value from the top of the stack.
  emitByte(OP_POP);
  // compiles the then branch.
  statement();

  int elseJump = emitJump(OP_JUMP);

  patchJump(thenJump);
  emitByte(OP_POP);

  if (match(TOKEN_ELSE))
    statement();
  patchJump(elseJump);
}

/**
 * printStatement - compiles a print statement.
 */
static void printStatement()
{
  expression();
  consume(TOKEN_SEMICOLON, L"ከመረጃው በሗላ ';' ያስፈልጋል።");
  emitByte(OP_PRINT);
}

/**
 * returnStatement - compiles a return statement.
 */
static void returnStatement()
{
  if (current->type == TYPE_SCRIPT)
  {
    error(L"ከመጀመሪያ ደረጃ ኮድ መልስ መስጠት አይቻልም።");
  }
  if (match(TOKEN_SEMICOLON))
  {
    emitReturn();
  }
  else
  {
    if (current->type == TYPE_INITIALIZER)
    {
      error(L"ከማስጀመሪያ መረጃ መልስ መስጠት አይቻልም።");
    }

    expression();
    consume(TOKEN_SEMICOLON, L"ከመልስ በሗላ ';' ያስፈልጋል።");
    emitByte(OP_RETURN);
  }
}

/**
 * whileStatement - compiles a while statement.
 */
static void whileStatement()
{
  int loopStart = currentChunk()->count;

  consume(TOKEN_LEFT_PAREN, L"ከ'እስከ' በሗላ '(' ያስፈልጋል።");
  expression();
  consume(TOKEN_RIGHT_PAREN, L"ከ 'ሁኔታው' በሗላ ')' ያስፈልጋል።");

  int exitJump = emitJump(OP_JUMP_IF_FALSE);

  emitByte(OP_POP);
  statement();

  emitLoop(loopStart);

  patchJump(exitJump);
  emitByte(OP_POP);
}

/**
 * synchronize - synchronizes the parser after a syntax error.
 * @note: the parser will synchronize by skipping tokens until it finds a statement that can start at the current position.
 *       this is done to avoid cascading errors.
 */
static void synchronize()
{
  parser.panicMode = false;

  while (parser.current.type != TOKEN_EOF)
  {
    if (parser.previous.type == TOKEN_SEMICOLON)
      return;

    switch (parser.current.type)
    {
    case TOKEN_CLASS:
    case TOKEN_FUN:
    case TOKEN_VAR:
    case TOKEN_FOR:
    case TOKEN_IF:
    case TOKEN_WHILE:
    case TOKEN_PRINT:
    case TOKEN_RETURN:
      return;
    default:
        // Do nothing.
        ;
    }

    advance();
  }
}

/**
 * declaration - compiles a declaration.
 * @note: a declaration is a statement that introduces a new variable.
 *       it can be a variable declaration or a function declaration.
 */
static void declaration()
{
  if (match(TOKEN_CLASS))
  {
    classDeclaration();
  }
  else if (match(TOKEN_FUN))
  {
    funDeclaration();
  }
  else if (match(TOKEN_VAR))
  {
    varDeclaration();
  }
  else
  {
    statement();
  }

  if (parser.panicMode)
    synchronize();
}

static void statement()
{
  if (match(TOKEN_PRINT))
  {
    printStatement();
  }
  else if (match(TOKEN_FOR))
  {
    forStatement();
  }
  else if (match(TOKEN_IF))
  {
    ifStatement();
  }
  else if (match(TOKEN_RETURN))
  {
    returnStatement();
  }
  else if (match(TOKEN_WHILE))
  {
    whileStatement();
  }
  else if (match(TOKEN_LEFT_BRACE))
  {
    beginScope();
    block();
    endScope();
  }
  else
  {
    expressionStatement();
  }
}

/**
 * compile - initializes the compiler.
 * @source: the source code to compile.
 * @chunk: the chunk to initialize.
 * @return: true if the source code was compiled successfully, false otherwise.
 */
ObjFunction *compile(const wchar_t *source)
{
  initScanner(source);
  Compiler compiler;
  initCompiler(&compiler, TYPE_SCRIPT);

  parser.hadError = false;
  parser.panicMode = false;

  advance();

  while (!match(TOKEN_EOF))
  {
    declaration();
  }
  ObjFunction *function = endCompiler();
  return parser.hadError ? NULL : function;
}

void markCompilerRoots()
{
  Compiler *compiler = current;
  while (compiler != NULL)
  {
    markObject((Obj *)compiler->function);
    compiler = compiler->enclosing;
  }
}
