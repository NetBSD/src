/*	$NetBSD: msg_195.c,v 1.4 2023/07/07 19:45:22 rillig Exp $	*/
# 3 "msg_195.c"

// Test for message: case not in switch [195]

/* lint1-extra-flags: -X 351 */

int
example(int x)
{
	/* expect+1: error: case not in switch [195] */
case 1:
	/* expect+1: error: case not in switch [195] */
case 2:
	return x;
}
