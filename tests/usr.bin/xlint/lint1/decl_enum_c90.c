/*	$NetBSD: decl_enum_c90.c,v 1.2 2022/06/17 18:54:53 rillig Exp $	*/
# 3 "decl_enum_c90.c"

/*
 * Tests for enum declarations in C90 mode.
 */

/* lint1-flags: -sw */

enum {
	A,
};
/* expect-1: error: trailing ',' prohibited in enum declaration [54] */
