/*	$NetBSD: msg_330.c,v 1.1 2021/01/14 22:18:14 rillig Exp $	*/
# 3 "msg_330.c"

// Test for message: operand of '%s' must be bool, not '%s' [330]
//
// See d_c99_bool_strict.c for many more examples.

/* lint1-extra-flags: -T */

typedef _Bool bool;

void
called(bool);

/*ARGSUSED*/
void
example(bool b, char c, int i)
{
	called(!b);
	called(!c);                /* expect: 330 */
	called(!i);                /* expect: 330 */
}
