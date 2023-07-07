/*	$NetBSD: msg_110.c,v 1.4 2023/07/07 19:45:22 rillig Exp $	*/
# 3 "msg_110.c"

// Test for message: pointer to function is not allowed here [110]

/* lint1-extra-flags: -X 351 */

/* ARGSUSED */
void
call_take(int (*ptr)(int))
{
	/* expect+1: error: pointer to function is not allowed here [110] */
	ptr++;

	/* expect+1: error: pointer to function is not allowed here [110] */
	ptr + 1;
}
