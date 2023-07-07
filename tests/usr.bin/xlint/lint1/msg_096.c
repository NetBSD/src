/*	$NetBSD: msg_096.c,v 1.7 2023/07/07 19:45:22 rillig Exp $	*/
# 3 "msg_096.c"

// Test for message: cannot dereference non-pointer type '%s' [96]

/* lint1-extra-flags: -X 351 */

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

	/* expect+1: error: cannot dereference non-pointer type 'int' [96] */
	return *i;
}
