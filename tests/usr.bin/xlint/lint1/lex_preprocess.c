/*	$NetBSD: lex_preprocess.c,v 1.2.2.2 2026/11/04 23:28:26 perseant Exp $	*/
# 3 "lex_preprocess.c"

/*
 * Tests for tokenizing preprocessing lines.
 */

#define EXPRESSION_LIKE (1 + 2 + 3)

#define FUNCTION_LIKE(a, b) function((b), (a))

// Lint1 sees the translation unit after preprocessing, and at that stage,
// line continuations are already represented as a single space.
#define STATEMENT_LIKE() do { } while (false)

// Neither of the following characters is a preprocessing token in strict
// standard C, but the '$' occurs when generating assembler code.
#define REGISTER_PREFIX $
#define BACKTICK /* ` */
#define COMMERCIAL_AT /* @ */

typedef int dummy;
