/*	$NetBSD: msg_327.c,v 1.6 2022/06/17 06:59:16 rillig Exp $	*/
# 3 "msg_327.c"

/* Test for message: declarations after statements is a C99 feature [327] */

/* lint1-flags: -w */

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
