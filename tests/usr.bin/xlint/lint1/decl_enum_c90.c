/*	$NetBSD: decl_enum_c90.c,v 1.1 2021/07/15 21:00:05 rillig Exp $	*/
# 3 "decl_enum_c90.c"

/*
 * Tests for enum declarations in C90 mode.
 */

/* lint1-flags: -sw */

enum {
	A,
};
/* expect-1: trailing ',' prohibited in enum declaration [54] */
