/*	$NetBSD: msg_327.c,v 1.3 2021/03/20 14:13:51 rillig Exp $	*/
# 3 "msg_327.c"

/* Test for message: declarations after statements is a C9X feature [327] */

/* lint1-flags: -w */

void statement(void);

/*ARGSUSED*/
void
example(void)
{
	statement();
	int declaration_1;	/* FIXME: expect 327 */
	statement();
	int declaration_2;	/* expect: 327 */
	statement();
	int declaration_3;	/* expect: 327 */
}				/*FIXME*//* expect: syntax error '}' */

/*FIXME*//* expect+1: cannot recover */
