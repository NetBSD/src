/*	$NetBSD: msg_107.c,v 1.3 2022/06/16 16:58:36 rillig Exp $	*/
# 3 "msg_107.c"

// Test for message: operands of '%s' have incompatible types (%s != %s) [107]

/* ARGSUSED */
void
compare(double d, void *ptr)
{
	/* expect+1: error: operands of '==' have incompatible types (double != pointer) [107] */
	return d == ptr;
}
