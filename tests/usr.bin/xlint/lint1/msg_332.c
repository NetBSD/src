/*	$NetBSD: msg_332.c,v 1.6 2023/07/09 10:42:07 rillig Exp $	*/
# 3 "msg_332.c"

// Test for message: right operand of '%s' must be bool, not '%s' [332]
//
// See d_c99_bool_strict.c for many more examples.

/* lint1-extra-flags: -T -X 351 */

typedef _Bool bool;

void
test(bool);

void
example(bool b, char c, int i)
{
	test(b && b);

	/* expect+2: error: right operand of '&&' must be bool, not 'char' [332] */
	/* expect+1: error: argument 1 expects '_Bool', gets passed 'int' [334] */
	test(b && c);

	/* expect+2: error: right operand of '&&' must be bool, not 'int' [332] */
	/* expect+1: error: argument 1 expects '_Bool', gets passed 'int' [334] */
	test(b && i);

	test(c != '\0');
	test(i != 0);
}
