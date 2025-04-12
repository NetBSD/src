/*	$NetBSD: msg_184.c,v 1.7 2025/04/12 15:49:50 rillig Exp $	*/
# 3 "msg_184.c"

// Test for message: invalid combination of '%s' and '%s' [184]

/* lint1-extra-flags: -X 351 */

int *
example(char *cp)
{
	/* expect+1: warning: invalid combination of 'pointer to int' and 'pointer to char' [184] */
	return cp;
}
