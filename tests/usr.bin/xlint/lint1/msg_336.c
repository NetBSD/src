/*	$NetBSD: msg_336.c,v 1.6 2023/07/09 10:42:07 rillig Exp $	*/
# 3 "msg_336.c"

// Test for message: left operand of '%s' must not be bool [336]
//
// See d_c99_bool_strict.c for many more examples.

/* lint1-extra-flags: -T -X 351 */

typedef _Bool bool;

void
test(bool);

void
example(bool b, int i)
{
	/* expect+2: error: left operand of '+' must not be bool [336] */
	/* expect+1: error: argument 1 expects '_Bool', gets passed 'int' [334] */
	test(b + i);

	test(b);

	test(i != 0);
}
