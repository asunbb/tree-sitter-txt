#include "tree_sitter/parser.h"
#include <string.h>
#include <stdlib.h>

enum TokenType {
  NEWLINE,
  INDENT,
  DEDENT,
};

#define MAX_INDENT_STACK 128

typedef struct {
  uint32_t indent_stack[MAX_INDENT_STACK];
  uint32_t stack_size;
  uint32_t pending_dedents;
  bool newline_after_dedent;
} Scanner;

static void advance(TSLexer *lexer) { lexer->advance(lexer, false); }

static uint32_t current_level(Scanner *scanner) {
  if (scanner->stack_size == 0) return 0;
  return scanner->indent_stack[scanner->stack_size - 1];
}

void *tree_sitter_litetxt_external_scanner_create(void) {
  Scanner *scanner = calloc(1, sizeof(Scanner));
  scanner->indent_stack[0] = 0;
  scanner->stack_size = 1;
  return scanner;
}

void tree_sitter_litetxt_external_scanner_destroy(void *payload) { free(payload); }

unsigned tree_sitter_litetxt_external_scanner_serialize(void *payload,
                                                    char *buffer) {
  Scanner *scanner = (Scanner *)payload;
  uint32_t size = sizeof(uint32_t) + sizeof(uint32_t) + sizeof(bool) +
                  scanner->stack_size * sizeof(uint32_t);
  if (size > TREE_SITTER_SERIALIZATION_BUFFER_SIZE) return 0;
  unsigned offset = 0;
  memcpy(buffer + offset, &scanner->stack_size, sizeof(uint32_t));
  offset += sizeof(uint32_t);
  memcpy(buffer + offset, &scanner->pending_dedents, sizeof(uint32_t));
  offset += sizeof(uint32_t);
  memcpy(buffer + offset, &scanner->newline_after_dedent, sizeof(bool));
  offset += sizeof(bool);
  memcpy(buffer + offset, scanner->indent_stack,
         scanner->stack_size * sizeof(uint32_t));
  return size;
}

void tree_sitter_litetxt_external_scanner_deserialize(void *payload,
                                                  const char *buffer,
                                                  unsigned length) {
  Scanner *scanner = (Scanner *)payload;
  if (length > 0) {
    unsigned offset = 0;
    memcpy(&scanner->stack_size, buffer + offset, sizeof(uint32_t));
    offset += sizeof(uint32_t);
    memcpy(&scanner->pending_dedents, buffer + offset, sizeof(uint32_t));
    offset += sizeof(uint32_t);
    memcpy(&scanner->newline_after_dedent, buffer + offset, sizeof(bool));
    offset += sizeof(bool);
    memcpy(scanner->indent_stack, buffer + offset,
           scanner->stack_size * sizeof(uint32_t));
  } else {
    scanner->indent_stack[0] = 0;
    scanner->stack_size = 1;
    scanner->pending_dedents = 0;
    scanner->newline_after_dedent = false;
  }
}

bool tree_sitter_litetxt_external_scanner_scan(void *payload, TSLexer *lexer,
                                          const bool *valid_symbols) {
  Scanner *scanner = (Scanner *)payload;

  // Emit pending DEDENTs first
  if (scanner->pending_dedents > 0 && valid_symbols[DEDENT]) {
    scanner->pending_dedents--;
    lexer->result_symbol = DEDENT;
    return true;
  }

  // DEDENT consumed the newline already; we still need to emit a zero-width
  // NEWLINE so the grammar's seq($._newline, $.segment) can match the sibling
  // segment that follows the dedent
  if (scanner->newline_after_dedent) {
    if (valid_symbols[NEWLINE]) {
      scanner->newline_after_dedent = false;
      lexer->result_symbol = NEWLINE;
      return true;
    }
    scanner->newline_after_dedent = false;
  }

  uint32_t level = current_level(scanner);

  // At EOF, emit remaining DEDENTs
  if (lexer->eof(lexer)) {
    if (scanner->stack_size > 1 && valid_symbols[DEDENT]) {
      scanner->stack_size--;
      lexer->result_symbol = DEDENT;
      return true;
    }
    return false;
  }

  // Only process newlines
  if (lexer->lookahead != '\n')
    return false;

  // Consume the newline
  advance(lexer);

  // Skip blank lines entirely — they carry no structural meaning.
  // Count leading spaces of the next non-blank line to determine indent level.
  uint32_t spaces = 0;
  while (true) {
    spaces = 0;
    while (lexer->lookahead == ' ') {
      spaces++;
      advance(lexer);
    }

    if (lexer->lookahead == '\n') {
      advance(lexer);
      continue;
    }

    if (lexer->eof(lexer)) {
      if (scanner->stack_size > 1 && valid_symbols[DEDENT]) {
        scanner->stack_size--;
        lexer->result_symbol = DEDENT;
        return true;
      }
      return false;
    }

    break;
  }

  // 2 spaces = 1 indent level.
  // Jumping multiple levels (e.g. 0→6 spaces) produces a single INDENT —
  // same as Python: the actual indent depth is recorded on the stack.
  uint32_t new_level = spaces / 2;

  if (new_level > level) {
    // Indentation increased
    if (valid_symbols[INDENT]) {
      if (scanner->stack_size < MAX_INDENT_STACK) {
        scanner->indent_stack[scanner->stack_size++] = new_level;
      }
      lexer->result_symbol = INDENT;
      return true;
    }
  } else if (new_level < level) {
    // Indentation decreased: pop stack entries above new_level
    uint32_t pops = 0;
    while (scanner->stack_size > 1 &&
           scanner->indent_stack[scanner->stack_size - 1] > new_level) {
      scanner->stack_size--;
      pops++;
    }
    if (valid_symbols[DEDENT] && pops > 0) {
      scanner->pending_dedents = pops - 1;
      scanner->newline_after_dedent = true;
      lexer->result_symbol = DEDENT;
      return true;
    }
    if (valid_symbols[NEWLINE]) {
      lexer->result_symbol = NEWLINE;
      return true;
    }
  } else {
    // Same indentation level
    if (valid_symbols[NEWLINE]) {
      lexer->result_symbol = NEWLINE;
      return true;
    }
  }

  return false;
}
