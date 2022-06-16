/*	$NetBSD: msg_122.c,v 1.5 2022/06/16 16:58:36 rillig Exp $	*/
# 3 "msg_122.c"

// Test for message: shift amount %llu is greater than bit-size %llu of '%s' [122]

int
example(int x)
{
	/* expect+1: warning: shift amount 129 is greater than bit-size 32 of 'int' [122] */
	return x << 129;
}
