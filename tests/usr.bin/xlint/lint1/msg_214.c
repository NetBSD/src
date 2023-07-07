/*	$NetBSD: msg_214.c,v 1.5 2023/07/07 19:45:22 rillig Exp $	*/
# 3 "msg_214.c"

// Test for message: function '%s' expects to return value [214]

/* lint1-extra-flags: -X 351 */

int
int_function(void)
{
	/* expect+1: warning: function 'int_function' expects to return value [214] */
	return;
}
