/*	$NetBSD: msg_090.c,v 1.8 2024/01/28 08:17:27 rillig Exp $	*/
# 3 "msg_090.c"

// Test for message: inconsistent redeclaration of extern '%s' [90]

/* lint1-extra-flags: -X 351 */

extern int random_number(void);

void
use(void)
{
	/* expect+3: warning: nested 'extern' declaration of 'random_number' [352] */
	/* expect+2: warning: inconsistent redeclaration of extern 'random_number' [90] */
	/* expect+1: warning: 'random_number' unused in function 'use' [192] */
	extern int random_number(int);
}
