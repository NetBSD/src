/*	$NetBSD: msg_107.c,v 1.5 2023/06/03 20:28:54 rillig Exp $	*/
# 3 "msg_107.c"

// Test for message: operands of '%s' have incompatible types '%s' and '%s' [107]

/* ARGSUSED */
void
compare(double d, void *ptr)
{
	/* expect+1: error: operands of '==' have incompatible types 'double' and 'pointer to void' [107] */
	return d == ptr;
}
