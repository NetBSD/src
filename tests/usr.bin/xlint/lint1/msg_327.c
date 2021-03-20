/*	$NetBSD: msg_327.c,v 1.5 2021/03/20 15:28:07 rillig Exp $	*/
# 3 "msg_327.c"

/* Test for message: declarations after statements is a C99 feature [327] */

/* lint1-flags: -w */

void statement(void);

/*ARGSUSED*/
void
example(void)
{
	statement();
	int declaration_1;	/* expect: 327 */
	statement();
	int declaration_2;	/* expect: 327 */
	statement();
	int declaration_3;	/* expect: 327 */
}
