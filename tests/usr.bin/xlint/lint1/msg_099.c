/*	$NetBSD: msg_099.c,v 1.6 2023/07/07 19:45:22 rillig Exp $	*/
# 3 "msg_099.c"

// Test for message: '%s' undefined [99]

/* lint1-extra-flags: -X 351 */

void
example(int defined_variable)
{
	int ok = defined_variable;
	/* expect+1: error: 'undefined_variable' undefined [99] */
	int error = undefined_variable;
}
