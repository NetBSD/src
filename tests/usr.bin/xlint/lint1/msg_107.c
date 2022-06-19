/*	$NetBSD: msg_107.c,v 1.4 2022/06/19 12:14:34 rillig Exp $	*/
# 3 "msg_107.c"

// Test for message: operands of '%s' have incompatible types '%s' and '%s' [107]

/* ARGSUSED */
void
compare(double d, void *ptr)
{
	/* expect+1: error: operands of '==' have incompatible types 'double' and 'pointer' [107] */
	return d == ptr;
}
