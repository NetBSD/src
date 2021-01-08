/*	$NetBSD: msg_109.c,v 1.2 2021/01/08 21:25:03 rillig Exp $	*/
# 3 "msg_109.c"

// Test for message: void type illegal in expression [109]

int
example(int arg)
{
	return arg + (void)4;
}
