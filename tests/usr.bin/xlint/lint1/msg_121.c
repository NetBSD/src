/*	$NetBSD: msg_121.c,v 1.3 2021/01/31 11:12:07 rillig Exp $	*/
# 3 "msg_121.c"

// Test for message: negative shift [121]

int
example(int x)
{
	return x << (3 - 5);		/* expect: 121 */
}
