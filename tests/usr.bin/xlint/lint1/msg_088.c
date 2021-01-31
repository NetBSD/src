/*	$NetBSD: msg_088.c,v 1.3 2021/01/31 11:12:07 rillig Exp $	*/
# 3 "msg_088.c"

// Test for message: typedef hides external declaration: %s [88]

/* lint1-flags: -g -h -S -w */

extern int identifier;

void
func(void)
{
	typedef double identifier;	/* expect: 88 */
}
