/*	$NetBSD: parse_stmt_error.c,v 1.2 2022/06/17 18:54:53 rillig Exp $	*/
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
		/* expect+1: error: syntax error ']' [249] */
		];
}

void
cover_selection_statement_switch(int x)
{
	switch (x)
		/* expect+1: error: syntax error ']' [249] */
		];
}
