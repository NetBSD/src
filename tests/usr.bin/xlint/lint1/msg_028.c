/*	$NetBSD: msg_028.c,v 1.5 2023/07/07 19:45:22 rillig Exp $	*/
# 3 "msg_028.c"

// Test for message: redefinition of '%s' [28]

/* lint1-extra-flags: -X 351 */

int
defined(int arg)
{
	return arg;
}

int
defined(int arg)
/* expect+1: error: redefinition of 'defined' [28] */
{
	return arg;
}
