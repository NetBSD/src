/*	$NetBSD: parse_stmt_error.c,v 1.1 2021/07/25 09:29:20 rillig Exp $	*/
# 3 "parse_stmt_error.c"

/*
 * Test parsing of errors in selection statements (if, switch).
 */

void do_nothing(void);

void
cover_selection_statement_else(_Bool cond)
{
	if (cond)
		do_nothing();
	else
		/* expect+1: syntax error ']' [249] */
		];
}

void
cover_selection_statement_switch(int x)
{
	switch (x)
		/* expect+1: syntax error ']' [249] */
		];
}
