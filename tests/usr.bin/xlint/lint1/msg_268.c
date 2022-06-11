/*	$NetBSD: msg_268.c,v 1.4 2022/06/11 11:52:13 rillig Exp $	*/
# 3 "msg_268.c"

// Test for message: variable '%s' declared inline [268]

int
example(int arg)
{
	/* expect+1: warning: variable 'local' declared inline [268] */
	inline int local = arg;

	return local;
}
