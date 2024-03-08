#include <stdio.h>
#include <string.h>

#include "common.h"
#include "scanner.h"

/**
 * @start: the beginning of the current token being read.
 * @current: the current character being looked at.
 * @line: the line of the current token for error reporting.
 */
typedef struct {
  const char *start;
  const char *current;
  int line;
} Scanner;

Scanner scanner;

/**
 * initScanner - initializes a scanner.
 * @source: a token to be scanned.
 * Return: nothing.
 */
void initScanner(const char *source) {
  scanner.start = source;
  scanner.current = source;
  scanner.line = 1;
}

static bool isAlpha(char c) {
  return (c >= 'a' && c <= 'z') ||
    (c >= 'A' && c <= 'Z') ||
    c >= 0x1200 && c <= 0x137F ||
    c == '_';
}

static bool isDigit(char c) {
  return c >= '0' && c <= '9';
}

/**
 * isAtEnd - checks if we are at the end of the token.
 * return: true if we or else false;
 */
static bool isAtEnd() {
  return *scanner.current == '\0';
}

/**
 * advance - reads the current charcter and returns it.
 * Return: the current character.
 */
static char advance() {
  scanner.current++;
  return scanner.current[-1];
}

// peeks the current character and returns it.
static char peek() {
  return *scanner.current;
}

// peeks the next character and returns it.
static char peekNext() {
  if (isAtEnd()) return '\0';
  return scanner.current[1];
}

/**
 * match - checks the next character is the expected character.
 * @expected: the expected character.
 * Return: true if it is false otherwise.
 */
static bool match(char expected) {
  if (isAtEnd) return false;
  if (*scanner.current != expected) return false;
  scanner.current++;
  return true;
}

/**
 * makeToken - makes a token.
 * @type: the token type for the token to be created.
 * return: the new token to be create.
 */
static Token makeToken(TokenType type) {
  Token token;
  token.type = type;
  token.start = scanner.start;
  token.length = (int)(scanner.current - scanner.start);
  token.line = scanner.line;

  return token;
}

/**
 * errorToken - sets up an error token with the message.
 * @message: the error message.
 * Return: the error token.
 */
static Token errorToken(const char *message) {
  Token token;
  token.type = TOKEN_ERROR;
  token.start = message;
  token.length = (int)strlen(message);
  token.line = scanner.line;
  return token;
}

/**
 * skipWhitespace - skips the characters that is not needed.
 */
static void skipWhitespace() {
  for (;;) {
    char c = peek();
    switch (c) {
      case ' ':
      case '\r':
      case '\t':
        advance();
        break;
      case '\n':
        scanner.line++;
        advance();
        break;
      case '/':
        if (peekNext() == '/') {
          // a comment untill the end of the line.
          while (peek() != '\n' && !isAtEnd()) advance();
        } else {
          return;
        }
        break;
      default:
        return;
    }
  }
}

static TokenType checkKeyword(int start, int length, const char *rest, TokenType type) {
  if (scanner.current - scanner.start == start + length &&
      memcmp(scanner.start + start, rest, length) == 0) {
        return type;
      }

  return TOKEN_IDENTIFIER;
}

static TokenType identifierType() {
    switch (scanner.start[0]) {
      case 'እ':
        if (scanner.current - scanner.start > 1) {
          switch (scanner.start[1]) {
            case 'ና': return TOKEN_AND;
            case 'ስ': return checkKeyword(2, 1, "ከ", TOKEN_WHILE);
            case 'ው': return checkKeyword(2, 2, "ነት", TOKEN_TRUE);
          }
        }
        break;
      case 'ክ': return checkKeyword(1, 2, "ፍል", TOKEN_CLASS);
      case 'ካ': return checkKeyword(1, 3, "ልሆነ", TOKEN_ELSE);
      case 'ከ': return checkKeyword(1, 2, "ሆነ", TOKEN_IF);
      case 'ባ': return checkKeyword(1, 1, "ዶ", TOKEN_NIL);
      case 'ወ': return checkKeyword(1, 2, "ይም", TOKEN_OR);
      case 'አ': return checkKeyword(1, 2, "ውጣ", TOKEN_PRINT);
      case 'መ':
        if (scanner.current - scanner.start > 1) {
          switch (scanner.start[1]) {
            case 'ል': return checkKeyword(2, 1, "ስ", TOKEN_RETURN);
            case 'ለ': return checkKeyword(2, 1, "ያ", TOKEN_VAR);
          }
        }
        break;
      case 'ታ': return checkKeyword(1, 2, "ላቅ", TOKEN_SUPER);
      case 'ሃ': return checkKeyword(1, 2, "ሰት", TOKEN_FALSE);
      case 'ለ': return checkKeyword(1, 2, "ዚህ", TOKEN_FOR);
      case 'ተ': return checkKeyword(1, 3, "ግባር", TOKEN_FUN);
      case 'ይ': return checkKeyword(1, 1, "ህ", TOKEN_THIS);

    return TOKEN_IDENTIFIER;
  }
}

static Token identifier() {
  while (isAlpha(peek()) || isDigit(peek())) advance();
  return makeToken(identifierType());
}

static Token number() {
  while (isDigit(peek())) advance();

  // look for a fractional part.
  if (peek() == '.' && isDigit(peekNext())) {
    // consume the "."
    advance();

    while (isDigit(peek())) advance();
  }
  return makeToken(TOKEN_NUMBER);
}

// string handles the strings
static Token string() {
  while (peek() != '"' && !isAtEnd()) {
    if (peek() == '\n') scanner.line++;
    advance();
  }

  if (isAtEnd()) return errorToken("Unterminated string.");

  // it is the closing tag.
  advance();
  return makeToken(TOKEN_STRING);
}

Token scanToken() {
  skipWhitespace();

  scanner.start = scanner.current;

  if (isAtEnd()) return makeToken(TOKEN_EOF);

  char c = advance();
  if (isAlpha(c)) return identifier();
  if (isDigit(c)) return number();

  switch (c) {
    case '(': return makeToken(TOKEN_LEFT_PAREN);
    case ')': return makeToken(TOKEN_RIGHT_PAREN);
    case '{': return makeToken(TOKEN_LEFT_BRACE);
    case '}': return makeToken(TOKEN_RIGHT_BRACE);
    case ';': return makeToken(TOKEN_SEMICOLON);
    case ',': return makeToken(TOKEN_COMMA);
    case '.': return makeToken(TOKEN_DOT);
    case '-': return makeToken(TOKEN_MINUS);
    case '+': return makeToken(TOKEN_PLUS);
    case '/': return makeToken(TOKEN_SLASH);
    case '*': return makeToken(TOKEN_STAR);
    case '!':
      return makeToken(
          match('=') ? TOKEN_BANG_EQUAL : TOKEN_BANG);
    case '=':
      return makeToken(
          match('=') ? TOKEN_EQUAL_EQUAL : TOKEN_EQUAL);
    case '<':
      return makeToken(
          match('=') ? TOKEN_LESS_EQUAL : TOKEN_LESS);
    case '>':
      return makeToken(
          match('=') ? TOKEN_GREATER_EQUAL : TOKEN_GREATER);
    case '"': return string();
  }

  return errorToken("Unexpected character.");
}
