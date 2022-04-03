/*	$NetBSD: msg_099.c,v 1.5 2022/04/03 09:34:45 rillig Exp $	*/
# 3 "msg_099.c"

// Test for message: '%s' undefined [99]

void
example(int defined_variable)
{
	int ok = defined_variable;
	/* expect+1: error: 'undefined_variable' undefined [99] */
	int error = undefined_variable;
}
