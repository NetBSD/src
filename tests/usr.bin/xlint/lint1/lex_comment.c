/*	$NetBSD: lex_comment.c,v 1.1 2021/06/19 20:25:58 rillig Exp $	*/
# 3 "lex_comment.c"

/*
 * Before lex.c 1.41 from 2021-06-19, lint ran into an endless loop when it
 * saw an unclosed comment at the end of the translation unit.  In practice
 * this was not relevant since the translation unit always comes from the C
 * preprocessor, which always emits a well-formed token sequence.
 */

/* expect+3: error: unterminated comment [256] */
/* expect+2: warning: empty translation unit [272] */
/* unclosed comment
