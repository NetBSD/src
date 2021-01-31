/*	$NetBSD: msg_114.c,v 1.3 2021/01/31 11:12:07 rillig Exp $	*/
# 3 "msg_114.c"

// Test for message: %soperand of '%s' must be lvalue [114]

void
example(int a)			/* expect: 231 */
{
	3++;			/* expect: 114 */
	// FIXME: lint error: ../common/tyname.c, 190: tspec_name(0)
	// "string"++;
	(a + a)++;		/* expect: 114 */
}
