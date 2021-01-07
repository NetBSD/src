/*	$NetBSD: msg_096.c,v 1.2 2021/01/07 00:38:46 rillig Exp $	*/
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
unary_asterisk(int i)
{
	return *i;
}
