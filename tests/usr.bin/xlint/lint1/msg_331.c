/*	$NetBSD: msg_331.c,v 1.1 2021/01/14 22:18:14 rillig Exp $	*/
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
	test(c && b);		/* expect: 331 */
	test(i && b);		/* expect: 331 */

	test(c != '\0');
	test(i != 0);
}
