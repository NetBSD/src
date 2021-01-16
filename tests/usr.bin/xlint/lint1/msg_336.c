/*	$NetBSD: msg_336.c,v 1.2 2021/01/16 16:03:47 rillig Exp $	*/
# 3 "msg_336.c"

// Test for message: left operand of '%s' must not be bool [336]
//
// See d_c99_bool_strict.c for many more examples.

/* lint1-extra-flags: -T */

typedef _Bool bool;

void
test(bool);

void
example(bool b, int i)
{
	test(b + i);		/* expect: 336, 334 */
	test(b);
	test(i != 0);
}
