/*	$NetBSD: decl_enum.c,v 1.1 2021/07/15 21:00:05 rillig Exp $	*/
# 3 "decl_enum.c"

/*
 * Tests for enum declarations.
 */

/* cover 'enumerator_list: error' */
enum {
	/* expect+1: error: syntax error 'goto' [249] */
	goto
};

/* cover 'enum_specifier: enum error' */
/* expect+1: error: syntax error 'goto' [249] */
enum goto {
	A
};
/* expect-1: warning: empty declaration [0] */
