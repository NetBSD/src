/* $NetBSD: psym_if_expr_stmt.c,v 1.4.4.1 2025/08/02 05:58:13 perseant Exp $ */

/*
 * Tests for the parser symbol psym_if_expr_stmt, which represents the state
 * after reading the keyword 'if', the controlling expression and the
 * statement for the 'then' branch.
 *
 * At this point, the 'if' statement is not necessarily complete, it can be
 * completed with the keyword 'else' followed by a statement.
 *
 * Any token other than 'else' completes the 'if' statement.
 */

//indent input
void
function(void)
{
	if (cond)
		stmt();
	if (cond)
		stmt();
	else			/* belongs to the second 'if' */
		stmt();
}
//indent end

//indent run-equals-input


//indent input
{
for (ever1)
for (ever2)
for (ever3)
if (cond1)
if (cond2)
if (cond3)
return;

stmt;
}
//indent end

//indent run
{
	for (ever1)
		for (ever2)
			for (ever3)
				if (cond1)
					if (cond2)
						if (cond3)
							return;

	stmt;
}
//indent end
