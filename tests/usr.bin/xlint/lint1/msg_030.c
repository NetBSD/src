/*	$NetBSD: msg_030.c,v 1.7 2023/07/07 19:45:22 rillig Exp $	*/
# 3 "msg_030.c"

/* Test for message: redeclaration of '%s'; ANSI C requires static [30] */

/* lint1-flags: -sw -X 351 */

/* expect+1: error: old-style declaration; add 'int' [1] */
static a;
/* expect+1: warning: redeclaration of 'a'; ANSI C requires static [30] */
int a;

/* expect+1: error: old-style declaration; add 'int' [1] */
static b;
/* expect+1: warning: redeclaration of 'b'; ANSI C requires static [30] */
int b = 1;

/* expect+1: error: old-style declaration; add 'int' [1] */
static c = 1;
/* expect+1: warning: redeclaration of 'c'; ANSI C requires static [30] */
int c;

void
use_variables(void)
{
	c = a + b + c;
}
