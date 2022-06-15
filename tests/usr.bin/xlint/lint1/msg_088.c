/*	$NetBSD: msg_088.c,v 1.4 2022/06/15 20:18:31 rillig Exp $	*/
# 3 "msg_088.c"

// Test for message: typedef hides external declaration: %s [88]

/* lint1-flags: -g -h -S -w */

extern int identifier;

void
func(void)
{
	/* expect+1: warning: typedef hides external declaration: identifier [88] */
	typedef double identifier;
}
