/*	$NetBSD: msg_088.c,v 1.6 2023/03/28 14:44:34 rillig Exp $	*/
# 3 "msg_088.c"

// Test for message: typedef '%s' hides external declaration [88]

/* lint1-flags: -g -h -S -w -X 351 */

extern int identifier;

void
func(void)
{
	/* expect+1: warning: typedef 'identifier' hides external declaration [88] */
	typedef double identifier;
}
