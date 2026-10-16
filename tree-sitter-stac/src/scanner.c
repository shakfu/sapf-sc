// External scanner for tree-sitter-stac: the end of an import statement, which parseImport
// (src/engine/Parser.cpp) finds at a newline, a comment or the end of input.

#include "tree_sitter/parser.h"

enum TokenType { IMPORT_END };

void *tree_sitter_stac_external_scanner_create(void) { return NULL; }
void tree_sitter_stac_external_scanner_destroy(void *payload) { (void)payload; }
unsigned tree_sitter_stac_external_scanner_serialize(void *payload, char *buffer) { (void)payload; (void)buffer; return 0; }
void tree_sitter_stac_external_scanner_deserialize(void *payload, const char *buffer, unsigned length)
{
	(void)payload; (void)buffer; (void)length;
}

bool tree_sitter_stac_external_scanner_scan(void *payload, TSLexer *lexer, const bool *valid_symbols)
{
	(void)payload;
	if (!valid_symbols[IMPORT_END]) return false;
	while (lexer->lookahead == ' ' || lexer->lookahead == '\t') lexer->advance(lexer, true);
	int32_t c = lexer->lookahead;
	if (c == '\n' || c == '\r' || c == ';' || lexer->eof(lexer)) {
		lexer->result_symbol = IMPORT_END;
		lexer->mark_end(lexer);  // zero width: the newline or comment stays for the extras
		return true;
	}
	return false;
}
