/*	$NetBSD: msg_096.c,v 1.3 2021/01/31 11:12:07 rillig Exp $	*/
# 3 "msg_096.c"

// Test for message: cannot dereference non-pointer type [96]

int
unary_plus(int i)
{
	return +i;
}

int
unary_minus(int i)
{
	return -i;
}

int
unary_asterisk(int i)		/* expect: 231 */
{
	return *i;		/* expect: 96, 214 */
}
