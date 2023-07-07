/*	$NetBSD: msg_149.c,v 1.5 2023/07/07 19:45:22 rillig Exp $	*/
# 3 "msg_149.c"

// Test for message: cannot call '%s', must be a function [149]

/* lint1-extra-flags: -X 351 */

void
example(int i)
{
	i++;
	/* expect+1: error: cannot call 'int', must be a function [149] */
	i(3);
}
