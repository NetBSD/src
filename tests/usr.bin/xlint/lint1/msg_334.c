/*	$NetBSD: msg_334.c,v 1.1 2021/01/14 22:18:14 rillig Exp $	*/
# 3 "msg_334.c"

// Test for message: argument #%d expects '%s', gets passed '%s' [334]
//
// See d_c99_bool_strict.c for many more examples.

/* lint1-extra-flags: -T */

typedef _Bool bool;

void
test_bool(bool);
void
test_int(int);

void
caller(bool b, int i)
{
	test_bool(b);
	test_bool(i);		/* expect: 334 */
	test_int(b);		/* expect: 334 */
	test_int(i);
}
