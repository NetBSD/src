/*	$NetBSD: msg_028.c,v 1.3 2022/04/05 23:09:19 rillig Exp $	*/
# 3 "msg_028.c"

// Test for message: redefinition of %s [28]

int
defined(int arg)
{
	return arg;
}

int
defined(int arg)
/* expect+1: error: redefinition of defined [28] */
{
	return arg;
}
