/*	$NetBSD: msg_184.c,v 1.4 2021/08/14 13:00:55 rillig Exp $	*/
# 3 "msg_184.c"

// Test for message: illegal combination of '%s' and '%s' [184]

int *
example(char *cp)
{
	/* expect+1: illegal combination of 'pointer to int' and 'pointer to char' [184] */
	return cp;
}
