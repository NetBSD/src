/*	$NetBSD: msg_330.c,v 1.3 2021/01/16 16:03:47 rillig Exp $	*/
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
	called(!c);		/* expect: 330, 334 */
	called(!i);		/* expect: 330, 334 */
}
