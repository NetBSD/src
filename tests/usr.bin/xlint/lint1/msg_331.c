/*	$NetBSD: msg_331.c,v 1.6 2023/07/09 10:42:07 rillig Exp $	*/
# 3 "msg_331.c"

// Test for message: left operand of '%s' must be bool, not '%s' [331]
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

	/* expect+2: error: left operand of '&&' must be bool, not 'char' [331] */
	/* expect+1: error: argument 1 expects '_Bool', gets passed 'int' [334] */
	test(c && b);

	/* expect+2: error: left operand of '&&' must be bool, not 'int' [331] */
	/* expect+1: error: argument 1 expects '_Bool', gets passed 'int' [334] */
	test(i && b);

	test(c != '\0');
	test(i != 0);
}
