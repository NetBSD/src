/*	$NetBSD: msg_107.c,v 1.6 2023/07/07 19:45:22 rillig Exp $	*/
# 3 "msg_107.c"

// Test for message: operands of '%s' have incompatible types '%s' and '%s' [107]

/* lint1-extra-flags: -X 351 */

/* ARGSUSED */
void
compare(double d, void *ptr)
{
	/* expect+1: error: operands of '==' have incompatible types 'double' and 'pointer to void' [107] */
	return d == ptr;
}
