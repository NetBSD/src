/*	$NetBSD: msg_214.c,v 1.6 2023/08/06 19:44:50 rillig Exp $	*/
# 3 "msg_214.c"

// Test for message: function '%s' expects to return value [214]

/* lint1-extra-flags: -X 351 */

int
int_function(void)
{
	/* expect+1: error: function 'int_function' expects to return value [214] */
	return;
}
