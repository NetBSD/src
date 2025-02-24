/*	$NetBSD: msg_127.c,v 1.5 2025/02/24 19:56:27 rillig Exp $	*/
# 3 "msg_127.c"

/* Test for message: '&' before array or function: ignored [127] */
/*
 * This message is no longer used, as it contradicts all C standards and is
 * not mentioned in K&R 1978 either.
 */

/* lint1-extra-flags: -t */

void
example()
{
	if (&example != (void *)0)
		return;
}
