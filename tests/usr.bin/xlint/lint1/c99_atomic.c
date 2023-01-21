/*	$NetBSD: c99_atomic.c,v 1.1 2023/01/21 11:57:03 rillig Exp $	*/
# 3 "c99_atomic.c"

/*
 * The keyword '_Atomic' was added in C11.  This test ensures that in C99
 * mode, the parser recognizes the keyword but flags it.
 */

/* FIXME: The error messages are misleading. */

/* expect+2: error: old-style declaration; add 'int' [1] */
/* expect+1: error: syntax error 'int' [249] */
typedef _Atomic int atomic_int;

typedef _Atomic struct {
	int field;
} atomic_struct;
/* expect-1: error: illegal type combination [4] */
