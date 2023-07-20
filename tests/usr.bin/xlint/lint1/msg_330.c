/*	$NetBSD: msg_330.c,v 1.7 2023/07/09 10:42:07 rillig Exp $	*/
# 3 "msg_330.c"

// Test for message: operand of '%s' must be bool, not '%s' [330]
//
// See d_c99_bool_strict.c for many more examples.

/* lint1-extra-flags: -T -X 351 */

typedef _Bool bool;

void
called(bool);

/*ARGSUSED*/
void
example(bool b, char c, int i)
{
	called(!b);

	/* expect+2: error: operand of '!' must be bool, not 'char' [330] */
	/* expect+1: error: argument 1 expects '_Bool', gets passed 'int' [334] */
	called(!c);

	/* expect+2: error: operand of '!' must be bool, not 'int' [330] */
	/* expect+1: error: argument 1 expects '_Bool', gets passed 'int' [334] */
	called(!i);
}
