/*	$NetBSD: msg_016.c,v 1.4 2022/06/22 19:23:18 rillig Exp $	*/
# 3 "msg_016.c"

// Test for message: array of function is illegal [16]

typedef void function(void);

/* expect+1: error: array of function is illegal [16] */
function functions[] = {
	/*
	 * XXX: The below warning should not assume that function is an
	 * integer type.
	 */
	/* expect+1: warning: illegal combination of integer 'int' and pointer 'pointer to void' [183] */
	(void *)0,
};
