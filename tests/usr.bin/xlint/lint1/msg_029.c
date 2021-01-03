/*	$NetBSD: msg_029.c,v 1.2 2021/01/03 15:35:00 rillig Exp $	*/
# 3 "msg_029.c"

// Test for message: previously declared extern, becomes static: %s [29]

extern int function(void);

static int function(void)
{
	return function();
}
