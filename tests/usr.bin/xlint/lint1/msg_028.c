/*	$NetBSD: msg_028.c,v 1.4 2022/06/19 11:50:42 rillig Exp $	*/
# 3 "msg_028.c"

// Test for message: redefinition of '%s' [28]

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
