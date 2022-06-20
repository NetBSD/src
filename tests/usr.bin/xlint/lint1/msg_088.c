/*	$NetBSD: msg_088.c,v 1.5 2022/06/20 21:13:36 rillig Exp $	*/
# 3 "msg_088.c"

// Test for message: typedef '%s' hides external declaration [88]

/* lint1-flags: -g -h -S -w */

extern int identifier;

void
func(void)
{
	/* expect+1: warning: typedef 'identifier' hides external declaration [88] */
	typedef double identifier;
}
