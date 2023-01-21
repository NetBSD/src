/*	$NetBSD: c11_atomic.c,v 1.1 2023/01/21 11:57:03 rillig Exp $	*/
# 3 "c11_atomic.c"

/*
 * The keyword '_Atomic' was added in C11.  This test ensures that in C11
 * mode, '_Atomic' can be used as both type qualifier and type specifier.
 *
 * See also:
 *	C11 6.7.3 Type qualifiers
 *	C11 6.7.2.4 Atomic type specifiers
 */

/* lint1-extra-flags: -Ac11 */

/* FIXME: The error messages are misleading. */

/* C11 6.7.3 "Type qualifiers" */
/* expect+2: error: old-style declaration; add 'int' [1] */
/* expect+1: error: syntax error 'int' [249] */
typedef _Atomic int atomic_int;

typedef _Atomic struct {
	int field;
} atomic_struct;
/* expect-1: error: illegal type combination [4] */

/* TODO: C11 6.7.2.4 "Atomic type specifiers" */
