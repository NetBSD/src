/*	$NetBSD: msg_184.c,v 1.6 2023/07/07 19:45:22 rillig Exp $	*/
# 3 "msg_184.c"

// Test for message: illegal combination of '%s' and '%s' [184]

/* lint1-extra-flags: -X 351 */

int *
example(char *cp)
{
	/* expect+1: warning: illegal combination of 'pointer to int' and 'pointer to char' [184] */
	return cp;
}
