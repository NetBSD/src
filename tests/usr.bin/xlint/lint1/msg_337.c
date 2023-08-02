/*	$NetBSD: msg_337.c,v 1.7 2023/08/02 18:51:25 rillig Exp $	*/
# 3 "msg_337.c"

// Test for message: right operand of '%s' must not be bool [337]
//
// See d_c99_bool_strict.c for many more examples.

/* lint1-extra-flags: -T -X 351 */

typedef _Bool bool;

void
test(bool);

void
example(bool b, int i)
{
	/* expect+2: error: right operand of '+' must not be bool [337] */
	/* expect+1: error: parameter 1 expects '_Bool', gets passed 'int' [334] */
	test(i + b);

	test(b);

	test(i != 0);
}
