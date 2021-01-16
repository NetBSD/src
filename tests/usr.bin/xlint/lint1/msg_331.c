/*	$NetBSD: msg_331.c,v 1.2 2021/01/16 16:03:47 rillig Exp $	*/
# 3 "msg_331.c"

// Test for message: left operand of '%s' must be bool, not '%s' [331]
//
// See d_c99_bool_strict.c for many more examples.

/* lint1-extra-flags: -T */

typedef _Bool bool;

void
test(bool);

void
example(bool b, char c, int i)
{
	test(b && b);
	test(c && b);		/* expect: 331, 334 */
	test(i && b);		/* expect: 331, 334 */

	test(c != '\0');
	test(i != 0);
}
