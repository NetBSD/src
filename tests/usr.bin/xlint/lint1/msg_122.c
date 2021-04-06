/*	$NetBSD: msg_122.c,v 1.4 2021/04/06 21:32:57 rillig Exp $	*/
# 3 "msg_122.c"

// Test for message: shift amount %llu is greater than bit-size %llu of '%s' [122]

int
example(int x)
{
	return x << 129;		/* expect: 122 */
}
