/*	$NetBSD: msg_052.c,v 1.6 2024/12/01 18:37:54 rillig Exp $	*/
# 3 "msg_052.c"

// Test for message: cannot initialize parameter '%s' [52]

/* lint1-extra-flags: -X 351 */

int
/* expect+1: warning: function definition with identifier list is obsolete in C23 [384] */
definition(i)
	/* expect+1: error: cannot initialize parameter 'i' [52] */
	int i = 3;
{
	return i;
}
