/*	$NetBSD: msg_088.c,v 1.6.2.1 2025/08/02 05:58:16 perseant Exp $	*/
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
