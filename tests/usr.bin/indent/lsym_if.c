/* $NetBSD: lsym_if.c,v 1.5 2023/06/10 16:43:56 rillig Exp $ */

/*
 * Tests for the token lsym_if, which represents the keyword 'if' that starts
 * an 'if' or 'if-else' statement.
 */

//indent input
void
function(void)
{
	if(cond)stmt();
}
//indent end

//indent run
void
function(void)
{
	if (cond)
		stmt();
}
//indent end


/*
 * After an 'if' statement without an 'else' branch, braces start a separate
 * block.
 */
//indent input
{
	if(0)if(1)if(2)stmt();{}
}
//indent end

//indent run
{
	if (0)
		if (1)
			if (2)
// $ FIXME: The '{' must be on a separate line, with indentation 8.
				stmt(); {
				}
}
//indent end
