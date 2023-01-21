/*	$NetBSD: c99_atomic.c,v 1.2 2023/01/21 13:07:22 rillig Exp $	*/
# 3 "c99_atomic.c"

/*
 * The keyword '_Atomic' was added in C11.  This test ensures that in C99
 * mode, the parser recognizes the keyword but flags it.
 */

/* expect+1: error: '_Atomic' requires C11 or later [350] */
typedef _Atomic int atomic_int;

/* expect+1: error: '_Atomic' requires C11 or later [350] */
typedef _Atomic struct {
	int field;
} atomic_struct;
