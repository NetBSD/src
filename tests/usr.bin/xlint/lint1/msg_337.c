/*	$NetBSD: msg_337.c,v 1.3 2021/04/05 01:35:34 rillig Exp $	*/
# 3 "msg_337.c"

// Test for message: right operand of '%s' must not be bool [337]
//
// See d_c99_bool_strict.c for many more examples.

/* lint1-extra-flags: -T */

typedef _Bool bool;

void
test(bool);

void
example(bool b, int i)
{
	test(i + b);		/* expect: 337 *//* expect: 334 */
	test(b);
	test(i != 0);
}
