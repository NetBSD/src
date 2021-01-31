/*	$NetBSD: msg_099.c,v 1.3 2021/01/31 11:12:07 rillig Exp $	*/
# 3 "msg_099.c"

// Test for message: %s undefined [99]

void
example(int defined_variable)
{
	int ok = defined_variable;
	int error = undefined_variable;	/* expect: 99 */
}
