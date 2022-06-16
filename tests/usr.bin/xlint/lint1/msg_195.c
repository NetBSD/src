/*	$NetBSD: msg_195.c,v 1.3 2022/06/16 16:58:36 rillig Exp $	*/
# 3 "msg_195.c"

// Test for message: case not in switch [195]

int
example(int x)
{
	/* expect+1: error: case not in switch [195] */
case 1:
	/* expect+1: error: case not in switch [195] */
case 2:
	return x;
}
