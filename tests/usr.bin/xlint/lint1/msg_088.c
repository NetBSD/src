/*	$NetBSD: msg_088.c,v 1.2 2021/01/03 15:35:00 rillig Exp $	*/
# 3 "msg_088.c"

// Test for message: typedef hides external declaration: %s [88]

/* lint1-flags: -g -h -S -w */

extern int identifier;

void
func(void)
{
	typedef double identifier;
}
