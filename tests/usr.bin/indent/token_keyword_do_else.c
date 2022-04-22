/* $NetBSD: token_keyword_do_else.c,v 1.2 2022/04/22 21:21:20 rillig Exp $ */

/*
 * Tests for the keyword 'do' or 'else'.  These two keywords are followed by
 * a space.  In contrast to 'for', 'if' and 'while', the space is not
 * followed by a parenthesized expression.
 */

#indent input
void
function(void)
{
	do(var)--;while(var>0);
	if(var>0)var=0;else(var=3);
}
#indent end

#indent run
void
function(void)
{
	do
		(var)--;
	while (var > 0);
	if (var > 0)
		var = 0;
	else
		(var = 3);
}
#indent end
