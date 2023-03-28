/*	$NetBSD: msg_327.c,v 1.7 2023/03/28 14:44:35 rillig Exp $	*/
# 3 "msg_327.c"

/* Test for message: declarations after statements is a C99 feature [327] */

/* lint1-flags: -w -X 351 */

void statement(void);

/*ARGSUSED*/
void
example(void)
{
	statement();
	/* expect+1: warning: declarations after statements is a C99 feature [327] */
	int declaration_1;
	statement();
	/* expect+1: warning: declarations after statements is a C99 feature [327] */
	int declaration_2;
	statement();
	/* expect+1: warning: declarations after statements is a C99 feature [327] */
	int declaration_3;
}
