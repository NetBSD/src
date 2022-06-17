/*	$NetBSD: parse_stmt_iter_error.c,v 1.2 2022/06/17 18:54:53 rillig Exp $	*/
# 3 "parse_stmt_iter_error.c"

/*
 * Test parsing of errors in iteration statements (while, do, for).
 */

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
