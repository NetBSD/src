/*	$NetBSD: msg_147.c,v 1.3 2021/07/25 10:26:46 rillig Exp $	*/
# 3 "msg_147.c"

// Test for message: invalid cast expression [147]

// The type name 'int(int)' is a 'function(int) returning int'.
void take(int(int));

/* ARGSUSED */
void
call_take(int (*ptr)(int))
{
	/* XXX: That's a little too unspecific. */
	/* expect+1: error: invalid cast expression [147] */
	take((int(int))ptr);
}
