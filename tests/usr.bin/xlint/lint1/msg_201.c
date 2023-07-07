/*	$NetBSD: msg_201.c,v 1.4 2023/07/07 19:45:22 rillig Exp $	*/
# 3 "msg_201.c"

// Test for message: default outside switch [201]

/* lint1-extra-flags: -X 351 */

void
example(void)
{
	/* expect+1: error: default outside switch [201] */
default:
	return;
}
