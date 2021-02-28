/*	$NetBSD: msg_126.c,v 1.4 2021/02/28 12:40:00 rillig Exp $	*/
# 3 "msg_126.c"

// Test for message: incompatible types '%s' and '%s' in conditional [126]

int
max(int cond, void *ptr, double dbl)	/* expect: 231, 231, 231 */
{
	return cond ? ptr : dbl;	/* expect: 126, 214 */
}
