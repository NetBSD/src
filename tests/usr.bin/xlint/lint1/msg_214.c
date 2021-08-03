/*	$NetBSD: msg_214.c,v 1.3 2021/08/03 18:38:02 rillig Exp $	*/
# 3 "msg_214.c"

// Test for message: function %s expects to return value [214]

int
int_function(void)
{
	/* TODO: add quotes around '%s' */
	/* expect+1: warning: function int_function expects to return value [214] */
	return;
}
