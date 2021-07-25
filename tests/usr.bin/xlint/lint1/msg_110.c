/*	$NetBSD: msg_110.c,v 1.3 2021/07/25 10:26:46 rillig Exp $	*/
# 3 "msg_110.c"

// Test for message: pointer to function is not allowed here [110]

/* ARGSUSED */
void
call_take(int (*ptr)(int))
{
	/* expect+1: error: pointer to function is not allowed here [110] */
	ptr++;

	/* expect+1: error: pointer to function is not allowed here [110] */
	ptr + 1;
}
