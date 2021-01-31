/*	$NetBSD: msg_114.c,v 1.4 2021/01/31 16:00:05 rillig Exp $	*/
# 3 "msg_114.c"

// Test for message: %soperand of '%s' must be lvalue [114]

void
example(int a)			/* expect: 231 */
{
	3++;			/* expect: 114 */

	/*
	 * Before tree.c 1.137 from 2021-01-09, trying to increment an array
	 * aborted lint with 'common/tyname.c, 190: tspec_name(0)'.
	 *
	 * See msg_108.c for more details.
	 */
	"string"++;		/* expect: 108 */

	(a + a)++;		/* expect: 114 */
}
