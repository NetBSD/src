/*	$NetBSD: msg_214.c,v 1.4 2021/08/03 18:44:33 rillig Exp $	*/
# 3 "msg_214.c"

// Test for message: function '%s' expects to return value [214]

int
int_function(void)
{
	/* expect+1: warning: function 'int_function' expects to return value [214] */
	return;
}
