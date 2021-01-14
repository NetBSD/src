/*	$NetBSD: msg_332.c,v 1.1 2021/01/14 22:18:14 rillig Exp $	*/
# 3 "msg_332.c"

// Test for message: right operand of '%s' must be bool, not '%s' [332]
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
	test(b && c);		/* expect: 332 */
	test(b && i);		/* expect: 332 */

	test(c != '\0');
	test(i != 0);
}
