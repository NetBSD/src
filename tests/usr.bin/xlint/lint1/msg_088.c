/*	$NetBSD: msg_088.c,v 1.7 2024/10/14 18:43:24 rillig Exp $	*/
# 3 "msg_088.c"

// Test for message: typedef '%s' hides external declaration with type '%s' [88]

/* lint1-flags: -g -h -S -w -X 351 */

extern int identifier;

void
func(void)
{
	/* expect+1: warning: typedef 'identifier' hides external declaration with type 'int' [88] */
	typedef double identifier;
}
