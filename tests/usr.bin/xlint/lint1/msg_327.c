/*	$NetBSD: msg_327.c,v 1.8 2023/07/07 06:03:31 rillig Exp $	*/
# 3 "msg_327.c"

/* Test for message: declarations after statements is a C99 feature [327] */

/* lint1-flags: -w -X 192,351 */

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
