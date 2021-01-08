/*	$NetBSD: msg_122.c,v 1.2 2021/01/08 21:25:03 rillig Exp $	*/
# 3 "msg_122.c"

// Test for message: shift greater than size of object [122]

int
example(int x)
{
	return x << 129;
}
