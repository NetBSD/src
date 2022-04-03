/*	$NetBSD: msg_096.c,v 1.5 2022/04/03 09:34:45 rillig Exp $	*/
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

void
unary_asterisk(int i)
{
	i++;

	/* expect+1: error: cannot dereference non-pointer type [96] */
	return *i;
}
