/*	$NetBSD: msg_149.c,v 1.4 2022/06/22 19:23:18 rillig Exp $	*/
# 3 "msg_149.c"

// Test for message: cannot call '%s', must be a function [149]

void
example(int i)
{
	i++;
	/* expect+1: error: cannot call 'int', must be a function [149] */
	i(3);
}
