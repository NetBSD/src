/*	$NetBSD: msg_126.c,v 1.3 2021/01/31 11:12:07 rillig Exp $	*/
# 3 "msg_126.c"

// Test for message: incompatible types in conditional [126]

int
max(int cond, void *ptr, double dbl)	/* expect: 231, 231, 231 */
{
	return cond ? ptr : dbl;	/* expect: 126, 214 */
}
