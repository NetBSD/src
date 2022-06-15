/*	$NetBSD: msg_029.c,v 1.4 2022/06/15 20:18:31 rillig Exp $	*/
# 3 "msg_029.c"

// Test for message: previously declared extern, becomes static: %s [29]

extern int function(void);

static int function(void)
/* expect+1: warning: previously declared extern, becomes static: function [29] */
{
	return function();
}
