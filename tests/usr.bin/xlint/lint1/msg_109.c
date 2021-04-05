/*	$NetBSD: msg_109.c,v 1.4 2021/04/05 01:35:34 rillig Exp $	*/
# 3 "msg_109.c"

// Test for message: void type illegal in expression [109]

int
example(int arg)			/* expect: 231 */
{
	return arg + (void)4;		/* expect: 109 *//* expect: 214 */
}
