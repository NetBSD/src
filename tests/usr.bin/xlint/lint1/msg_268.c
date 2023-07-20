/*	$NetBSD: msg_268.c,v 1.5 2023/07/07 19:45:22 rillig Exp $	*/
# 3 "msg_268.c"

// Test for message: variable '%s' declared inline [268]

/* lint1-extra-flags: -X 351 */

int
example(int arg)
{
	/* expect+1: warning: variable 'local' declared inline [268] */
	inline int local = arg;

	return local;
}
