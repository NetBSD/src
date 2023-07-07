/*	$NetBSD: msg_228.c,v 1.4 2023/07/07 19:45:22 rillig Exp $	*/
# 3 "msg_228.c"

/* Test for message: function cannot return const or volatile object [228] */

/* TODO: Also warn in C99 mode and later. */
/* lint1-flags: -sw -X 351 */

const int
return_const_int(void)
/* expect+1: warning: function cannot return const or volatile object [228] */
{
	return 3;
}

volatile int
return_volatile_int(void)
/* expect+1: warning: function cannot return const or volatile object [228] */
{
	return 3;
}
