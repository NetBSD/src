/*	$NetBSD: msg_052.c,v 1.5 2023/07/07 19:45:22 rillig Exp $	*/
# 3 "msg_052.c"

// Test for message: cannot initialize parameter '%s' [52]

/* lint1-extra-flags: -X 351 */

int
definition(i)
	/* expect+1: error: cannot initialize parameter 'i' [52] */
	int i = 3;
{
	return i;
}
