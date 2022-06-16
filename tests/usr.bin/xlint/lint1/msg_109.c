/*	$NetBSD: msg_109.c,v 1.5 2022/06/16 16:58:36 rillig Exp $	*/
# 3 "msg_109.c"

// Test for message: void type illegal in expression [109]

/* ARGSUSED */
int
example(int arg)
{
	/* expect+2: error: void type illegal in expression [109] */
	/* expect+1: warning: function 'example' expects to return value [214] */
	return arg + (void)4;
}
