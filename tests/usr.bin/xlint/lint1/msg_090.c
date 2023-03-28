/*	$NetBSD: msg_090.c,v 1.6 2023/03/28 20:04:52 rillig Exp $	*/
# 3 "msg_090.c"

// Test for message: inconsistent redeclaration of extern '%s' [90]

/* lint1-extra-flags: -X 351 */

extern int random_number(void);

void
use(void)
{
	/* expect+2: warning: nested 'extern' declaration of 'random_number' [352] */
	/* expect+1: warning: inconsistent redeclaration of extern 'random_number' [90] */
	extern int random_number(int);
}
