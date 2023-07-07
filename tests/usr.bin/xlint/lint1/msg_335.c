/*	$NetBSD: msg_335.c,v 1.3 2023/07/07 19:45:22 rillig Exp $	*/
# 3 "msg_335.c"

// Test for message: operand of '%s' must not be bool [335]
//
// See d_c99_bool_strict.c for many more examples.

/* lint1-extra-flags: -T -X 351 */

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
