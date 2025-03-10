/*	$NetBSD: msg_127.c,v 1.6 2025/03/10 22:35:02 rillig Exp $	*/
# 3 "msg_127.c"

/* Test for message: '&' before array or function: ignored [127] */
/* This message is not used. */
/*
 * This message contradicts all C standards and is not mentioned in K&R 1978
 * either.
 */

/* lint1-extra-flags: -t */

void
example()
{
	if (&example != (void *)0)
		return;
}
