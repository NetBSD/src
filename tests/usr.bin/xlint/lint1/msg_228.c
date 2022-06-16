/*	$NetBSD: msg_228.c,v 1.3 2022/06/16 21:24:41 rillig Exp $	*/
# 3 "msg_228.c"

/* Test for message: function cannot return const or volatile object [228] */

/* TODO: Also warn in C99 mode and later. */
/* lint1-flags: -sw */

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
