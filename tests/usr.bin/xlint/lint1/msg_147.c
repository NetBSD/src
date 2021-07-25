/*	$NetBSD: msg_147.c,v 1.4 2021/07/25 10:39:10 rillig Exp $	*/
# 3 "msg_147.c"

// Test for message: invalid cast from '%s' to '%s' [147]

// The type name 'int(int)' is a 'function(int) returning int'.
void take(int(int));

/* ARGSUSED */
void
call_take(int (*ptr)(int))
{
	/* expect+1: error: invalid cast from 'pointer to function(int) returning int' to 'function(int) returning int' [147] */
	take((int(int))ptr);
}
