/*	$NetBSD: msg_030.c,v 1.3 2022/04/05 23:09:19 rillig Exp $	*/
# 3 "msg_030.c"

/* Test for message: redeclaration of %s; ANSI C requires static [30] */

/* lint1-flags: -sw */

static a;
/* expect+1: warning: redeclaration of a; ANSI C requires static [30] */
int a;

static b;
/* expect+1: warning: redeclaration of b; ANSI C requires static [30] */
int b = 1;

static c = 1;
/* expect+1: warning: redeclaration of c; ANSI C requires static [30] */
int c;

void
use_variables(void)
{
	c = a + b + c;
}
