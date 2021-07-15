/*	$NetBSD: decl_struct_c90.c,v 1.2 2021/07/15 20:05:49 rillig Exp $	*/
# 3 "decl_struct_c90.c"

/*
 * Test declaration of struct types, in C90 without any GNU extensions.
 */

/* lint1-flags: -sw */

/*
 * All of K&R, C90, C99 require that a struct member declaration is
 * terminated with a semicolon.
 *
 * Before cgram.y 1.328 from 2021-07-15, lint allowed the missing semicolon
 * in non-C90 mode, no idea why.
 */
struct missing_semicolon {
	int member
};
/* expect-1: error: syntax error '}' [249] */
/* expect+1: error: cannot recover from previous errors [224] */
