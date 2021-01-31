/*	$NetBSD: msg_109.c,v 1.3 2021/01/31 11:12:07 rillig Exp $	*/
# 3 "msg_109.c"

// Test for message: void type illegal in expression [109]

int
example(int arg)			/* expect: 231 */
{
	return arg + (void)4;		/* expect: 109, 214 */
}
