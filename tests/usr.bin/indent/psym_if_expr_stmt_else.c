/* $NetBSD: psym_if_expr_stmt_else.c,v 1.3 2022/04/23 09:01:03 rillig Exp $ */

/*
 * Tests for the parser symbol psym_if_expr_stmt_else, which represents the
 * parser state after reading the keyword 'if', the controlling expression,
 * the statement of the 'then' branch and the keyword 'else'.
 *
 * If the next token is an 'if', the formatting depends on the option '-ei' or
 * '-nei'.  Any other lookahead token completes the 'if' statement.
 */

#indent input
void
example(_Bool cond)
{
	if (cond) {}
	else if (cond) {}
	else if (cond) i++;
	else {}
}
#indent end

#indent run
void
example(_Bool cond)
{
	if (cond) {
	} else if (cond) {
	} else if (cond)
		i++;
	else {
	}
}
#indent end

/*
 * Combining the options '-bl' (place brace on the left margin) and '-ce'
 * (cuddle else) looks strange, but is technically correct.
 */
#indent run -bl
void
example(_Bool cond)
{
	if (cond)
	{
	} else if (cond)
	{
	} else if (cond)
		i++;
	else
	{
	}
}
#indent end

#indent run -bl -nce
void
example(_Bool cond)
{
	if (cond)
	{
	}
	else if (cond)
	{
	}
	else if (cond)
		i++;
	else
	{
	}
}
#indent end

/*
 * Adding the option '-nei' (do not join 'else if') expands the code even
 * more.
 */
#indent run -bl -nce -nei
void
example(_Bool cond)
{
	if (cond)
	{
	}
	else
		if (cond)
		{
		}
		else
			if (cond)
				i++;
			else
			{
			}
}
#indent end
