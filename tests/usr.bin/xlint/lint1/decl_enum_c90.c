/*	$NetBSD: decl_enum_c90.c,v 1.3 2023/07/11 20:54:23 rillig Exp $	*/
# 3 "decl_enum_c90.c"

/*
 * Tests for enum declarations in C90 mode.
 */

/* lint1-flags: -sw */

enum {
	A,
};
/* expect-1: error: trailing ',' in enum declaration requires C99 or later [54] */
