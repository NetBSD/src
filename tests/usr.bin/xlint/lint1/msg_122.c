/*	$NetBSD: msg_122.c,v 1.3 2021/01/31 11:12:07 rillig Exp $	*/
# 3 "msg_122.c"

// Test for message: shift greater than size of object [122]

int
example(int x)
{
	return x << 129;		/* expect: 122 */
}
