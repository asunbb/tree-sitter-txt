#include "tree_sitter/parser.h"
#include <string.h>
#include <stdlib.h>

enum TokenType {
  NEWLINE,
  INDENT,
  DEDENT,
};

typedef struct {
  uint32_t indent_level;
  uint32_t pending_dedents;
  bool newline_after_dedent;
} Scanner;

static void advance(TSLexer *lexer) { lexer->advance(lexer, false); }

void *tree_sitter_litetxt_external_scanner_create(void) {
  Scanner *scanner = calloc(1, sizeof(Scanner));
  return scanner;
}

void tree_sitter_litetxt_external_scanner_destroy(void *payload) { free(payload); }

unsigned tree_sitter_litetxt_external_scanner_serialize(void *payload,
                                                    char *buffer) {
  Scanner *scanner = (Scanner *)payload;
  memcpy(buffer, scanner, sizeof(Scanner));
  return sizeof(Scanner);
}

void tree_sitter_litetxt_external_scanner_deserialize(void *payload,
                                                  const char *buffer,
                                                  unsigned length) {
  Scanner *scanner = (Scanner *)payload;
  if (length >= sizeof(Scanner)) {
    memcpy(scanner, buffer, sizeof(Scanner));
  } else {
    scanner->indent_level = 0;
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
    scanner->indent_level--;
    lexer->result_symbol = DEDENT;
    return true;
  }

  // DEDENT consumed the newline; emit zero-width NEWLINE for parent context
  if (scanner->newline_after_dedent) {
    if (valid_symbols[NEWLINE]) {
      scanner->newline_after_dedent = false;
      lexer->result_symbol = NEWLINE;
      return true;
    }
    scanner->newline_after_dedent = false;
  }

  // At EOF, emit remaining DEDENTs
  if (lexer->eof(lexer)) {
    if (scanner->indent_level > 0 && valid_symbols[DEDENT]) {
      scanner->indent_level--;
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

  // Skip blank lines, find next non-blank line and count its leading spaces
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
      if (scanner->indent_level > 0 && valid_symbols[DEDENT]) {
        scanner->indent_level--;
        lexer->result_symbol = DEDENT;
        return true;
      }
      return false;
    }

    break;
  }

  // 2 spaces = 1 indent level
  uint32_t new_level = spaces / 2;

  if (new_level > scanner->indent_level) {
    // Indentation increased
    if (valid_symbols[INDENT]) {
      scanner->indent_level = new_level;
      lexer->result_symbol = INDENT;
      return true;
    }
  } else if (new_level < scanner->indent_level) {
    // Indentation decreased
    if (valid_symbols[DEDENT]) {
      scanner->pending_dedents = scanner->indent_level - new_level - 1;
      scanner->newline_after_dedent = true;
      scanner->indent_level--;
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
