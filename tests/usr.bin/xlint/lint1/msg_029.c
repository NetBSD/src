/*	$NetBSD: msg_029.c,v 1.3 2021/01/31 11:12:07 rillig Exp $	*/
# 3 "msg_029.c"

// Test for message: previously declared extern, becomes static: %s [29]

extern int function(void);

static int function(void)
{				/* expect: 29 */
	return function();
}
