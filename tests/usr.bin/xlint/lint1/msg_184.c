/*	$NetBSD: msg_184.c,v 1.5 2022/06/17 18:54:53 rillig Exp $	*/
# 3 "msg_184.c"

// Test for message: illegal combination of '%s' and '%s' [184]

int *
example(char *cp)
{
	/* expect+1: warning: illegal combination of 'pointer to int' and 'pointer to char' [184] */
	return cp;
}
