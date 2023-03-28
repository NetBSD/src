/*	$NetBSD: parse_stmt_error.c,v 1.3 2023/03/28 14:44:35 rillig Exp $	*/
# 3 "parse_stmt_error.c"

/*
 * Test parsing of errors in selection statements (if, switch).
 */

/* lint1-extra-flags: -X 351 */

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
