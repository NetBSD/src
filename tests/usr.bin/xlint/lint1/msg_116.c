/*	$NetBSD: msg_116.c,v 1.2 2021/01/08 21:25:03 rillig Exp $	*/
# 3 "msg_116.c"

// Test for message: illegal pointer subtraction [116]

unsigned long
example(int *a, double *b)
{
	return a - b;
}
