/*	$NetBSD: msg_267.c,v 1.3 2021/04/09 16:37:18 rillig Exp $	*/
# 3 "msg_267.c"

// Test for message: shift equal to size of object [267]

int
shr32(unsigned int x)
{
	return x >> 32;		/* expect: 267 */
}

int
shl32(unsigned int x)
{
	return x << 32;		/* expect: 267 */
}
