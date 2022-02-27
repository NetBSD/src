/*	$NetBSD: decl_struct_c90.c,v 1.4 2022/02/27 11:40:30 rillig Exp $	*/
# 3 "decl_struct_c90.c"

/*
 * Test declaration of struct types, in C90 without any GNU extensions.
 */

/* lint1-flags: -sw */

struct unnamed_member {
	enum { A, B, C } tag;
	union {
		int a_value;
		void *b_value;
		void (*c_value)(void);
	};
	/* expect-1: warning: anonymous struct/union members is a C11 feature [49] */
};

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
