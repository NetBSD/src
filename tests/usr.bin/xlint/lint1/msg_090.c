/*	$NetBSD: msg_090.c,v 1.4 2022/06/21 21:18:30 rillig Exp $	*/
# 3 "msg_090.c"

// Test for message: inconsistent redeclaration of extern '%s' [90]

extern int random_number(void);

void
use(void)
{
	/* expect+1: warning: inconsistent redeclaration of extern 'random_number' [90] */
	extern int random_number(int);
}
