/*	$NetBSD: stmt_for.c,v 1.1 2021/06/19 19:59:02 rillig Exp $	*/
# 3 "stmt_for.c"

/*
 * Before func.c 1.111 from 2021-06-19, lint ran into an assertion failure:
 *
 * "dcs->d_next == NULL" failed in funcend at func.c:422
 */

void
test(void)
{
	for (0 0;		/* expect: syntax error '0' */
}

/* expect+1: cannot recover from previous errors */
