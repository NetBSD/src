/*	$NetBSD: msg_335.c,v 1.1 2021/01/14 22:18:14 rillig Exp $	*/
# 3 "msg_335.c"

// Test for message: operand of '%s' must not be bool [335]
//
// See d_c99_bool_strict.c for many more examples.

/* lint1-extra-flags: -T */

typedef _Bool bool;

void
example(bool b)
{
	b = +b;			/* expect: 335 */
	b = -b;			/* expect: 335 */
	b = !b;
	b++;			/* expect: 335 */
	++b;			/* expect: 335 */
	b--;			/* expect: 335 */
	--b;			/* expect: 335 */
}
