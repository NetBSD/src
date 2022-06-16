/*	$NetBSD: msg_267.c,v 1.4 2022/06/16 21:24:41 rillig Exp $	*/
# 3 "msg_267.c"

// Test for message: shift equal to size of object [267]

int
shr32(unsigned int x)
{
	/* expect+1: warning: shift equal to size of object [267] */
	return x >> 32;
}

int
shl32(unsigned int x)
{
	/* expect+1: warning: shift equal to size of object [267] */
	return x << 32;
}
