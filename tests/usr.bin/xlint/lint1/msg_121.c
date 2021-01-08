/*	$NetBSD: msg_121.c,v 1.2 2021/01/08 21:25:03 rillig Exp $	*/
# 3 "msg_121.c"

// Test for message: negative shift [121]

int
example(int x)
{
	return x << (3 - 5);
}
