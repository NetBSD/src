/*	$NetBSD: msg_334.c,v 1.4 2023/07/09 10:42:07 rillig Exp $	*/
# 3 "msg_334.c"

// Test for message: argument %d expects '%s', gets passed '%s' [334]
//
// See d_c99_bool_strict.c for many more examples.

/* lint1-extra-flags: -T -X 351 */

typedef _Bool bool;

void
test_bool(bool);
void
test_int(int);

void
caller(bool b, int i)
{
	test_bool(b);

	/* expect+1: error: argument 1 expects '_Bool', gets passed 'int' [334] */
	test_bool(i);

	/* expect+1: error: argument 1 expects 'int', gets passed '_Bool' [334] */
	test_int(b);

	test_int(i);
}
