/*	$NetBSD: msg_201.c,v 1.3 2021/08/26 19:23:25 rillig Exp $	*/
# 3 "msg_201.c"

// Test for message: default outside switch [201]

void
example(void)
{
	/* expect+1: error: default outside switch [201] */
default:
	return;
}
