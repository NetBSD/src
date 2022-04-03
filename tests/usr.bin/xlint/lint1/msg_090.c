/*	$NetBSD: msg_090.c,v 1.3 2022/04/03 09:34:45 rillig Exp $	*/
# 3 "msg_090.c"

// Test for message: inconsistent redeclaration of extern: %s [90]

extern int random_number(void);

void
use(void)
{
	/* expect+1: warning: inconsistent redeclaration of extern: random_number [90] */
	extern int random_number(int);
}
