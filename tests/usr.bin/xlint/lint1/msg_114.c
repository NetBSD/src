/*	$NetBSD: msg_114.c,v 1.7 2023/07/07 19:45:22 rillig Exp $	*/
# 3 "msg_114.c"

// Test for message: %soperand of '%s' must be lvalue [114]

/* lint1-extra-flags: -X 351 */

/* ARGSUSED */
void
example(int a)
{
	/* expect+1: error: operand of 'x++' must be lvalue [114] */
	3++;

	/*
	 * Before tree.c 1.137 from 2021-01-09, trying to increment an array
	 * aborted lint with 'common/tyname.c, 190: tspec_name(0)'.
	 *
	 * See msg_108.c for more details.
	 */
	/* expect+1: error: operand of 'x++' has invalid type 'array[7] of char' [108] */
	"string"++;

	/* expect+1: error: operand of 'x++' must be lvalue [114] */
	(a + a)++;
}
