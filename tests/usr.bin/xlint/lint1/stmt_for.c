/*	$NetBSD: stmt_for.c,v 1.3 2022/06/17 18:54:53 rillig Exp $	*/
# 3 "stmt_for.c"

/*
 * Before func.c 1.111 from 2021-06-19, lint ran into an assertion failure:
 *
 * "dcs->d_next == NULL" failed in funcend at func.c:422
 */

void
test(void)
{
	/* expect+1: error: syntax error '0' [249] */
	for (0 0;
}

/* expect+1: error: cannot recover from previous errors [224] */
