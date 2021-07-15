/*	$NetBSD: decl_struct_c90.c,v 1.1 2021/07/15 19:51:29 rillig Exp $	*/
# 3 "decl_struct_c90.c"

/*
 * Test declaration of struct types, in C90 without any GNU extensions.
 */

/* lint1-flags: -sw */

/*
 * All of K&R, C90, C99 require that a struct member declaration is
 * terminated with a semicolon.  No idea why lint allows the missing
 * semicolon in non-C90 mode.
 */
struct missing_semicolon {
	int member
};
/* expect-1: error: syntax requires ';' after last struct/union member [66] */
