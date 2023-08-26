/*	$NetBSD: msg_030.c,v 1.8 2023/08/26 10:43:53 rillig Exp $	*/
# 3 "msg_030.c"

/* Test for message: redeclaration of '%s'; C90 or later require static [30] */

/* lint1-flags: -sw -X 351 */

/* expect+1: error: old-style declaration; add 'int' [1] */
static a;
/* expect+1: warning: redeclaration of 'a'; C90 or later require static [30] */
int a;

/* expect+1: error: old-style declaration; add 'int' [1] */
static b;
/* expect+1: warning: redeclaration of 'b'; C90 or later require static [30] */
int b = 1;

/* expect+1: error: old-style declaration; add 'int' [1] */
static c = 1;
/* expect+1: warning: redeclaration of 'c'; C90 or later require static [30] */
int c;

void
use_variables(void)
{
	c = a + b + c;
}
