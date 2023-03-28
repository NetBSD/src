/*	$NetBSD: parse_stmt_iter_error.c,v 1.3 2023/03/28 14:44:35 rillig Exp $	*/
# 3 "parse_stmt_iter_error.c"

/*
 * Test parsing of errors in iteration statements (while, do, for).
 */

/* lint1-extra-flags: -X 351 */

void do_nothing(void);

void
cover_iteration_statement_while(_Bool cond)
{
	while (cond)
		/* expect+1: error: syntax error ']' [249] */
		];
}

void
cover_iteration_statement_do(void)
{
	do
		/* expect+1: error: syntax error ']' [249] */
		];
}

void
cover_iteration_statement_for(void)
{
	for (int i = 0; i < 10; i++)
		/* expect+1: error: syntax error ']' [249] */
		];
}
