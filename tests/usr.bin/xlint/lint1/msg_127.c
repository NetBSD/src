/*	$NetBSD: msg_127.c,v 1.4.4.1 2025/08/02 05:58:16 perseant Exp $	*/
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
