/*	$NetBSD: msg_029.c,v 1.5 2022/06/19 11:50:42 rillig Exp $	*/
# 3 "msg_029.c"

// Test for message: '%s' was previously declared extern, becomes static [29]

extern int function(void);

static int function(void)
/* expect+1: warning: 'function' was previously declared extern, becomes static [29] */
{
	return function();
}
