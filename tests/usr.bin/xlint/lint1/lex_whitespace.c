/*	$NetBSD: lex_whitespace.c,v 1.1 2021/09/10 19:40:18 rillig Exp $	*/
# 3 "lex_whitespace.c"

/*
 * Test tracking of the current position in the translation unit, with
 * spaces, tabs, vertical tabs and form feeds.
 *
 * Both vertical tab and form feed do not increment the line number.
 * Lint agrees with GCC and Clang here.
 */

/* expect+1: warning: typedef declares no type name [72] */
typedef;

	/* horizontal tab */
/* expect+1: warning: typedef declares no type name [72] */
typedef;

/* vertical tab */
/* expect+1: warning: typedef declares no type name [72] */
typedef;

/* form feed */
/* expect+1: warning: typedef declares no type name [72] */
typedef;
