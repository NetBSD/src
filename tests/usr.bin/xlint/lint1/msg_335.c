/*	$NetBSD: msg_335.c,v 1.2 2022/06/17 06:59:16 rillig Exp $	*/
# 3 "msg_335.c"

// Test for message: operand of '%s' must not be bool [335]
//
// See d_c99_bool_strict.c for many more examples.

/* lint1-extra-flags: -T */

typedef _Bool bool;

void
example(bool b)
{
	/* expect+1: error: operand of '+' must not be bool [335] */
	b = +b;

	/* expect+1: error: operand of '-' must not be bool [335] */
	b = -b;

	b = !b;

	/* expect+1: error: operand of 'x++' must not be bool [335] */
	b++;

	/* expect+1: error: operand of '++x' must not be bool [335] */
	++b;

	/* expect+1: error: operand of 'x--' must not be bool [335] */
	b--;

	/* expect+1: error: operand of '--x' must not be bool [335] */
	--b;
}
