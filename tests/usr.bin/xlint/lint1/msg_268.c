/*	$NetBSD: msg_268.c,v 1.3 2021/08/22 13:45:56 rillig Exp $	*/
# 3 "msg_268.c"

// Test for message: variable declared inline: %s [268]

int
example(int arg)
{
	/* expect+1: warning: variable declared inline: local [268] */
	inline int local = arg;

	return local;
}
