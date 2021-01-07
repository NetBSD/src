/*	$NetBSD: msg_099.c,v 1.2 2021/01/07 00:38:46 rillig Exp $	*/
# 3 "msg_099.c"

// Test for message: %s undefined [99]

void
example(int defined_variable)
{
	int ok = defined_variable;
	int error = undefined_variable;
}
