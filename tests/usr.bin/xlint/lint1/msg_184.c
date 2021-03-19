/*	$NetBSD: msg_184.c,v 1.3 2021/03/19 08:01:58 rillig Exp $	*/
# 3 "msg_184.c"

// Test for message: illegal pointer combination [184]

int *
example(char *cp)
{
	return cp;		/* expect: 184 */
}
