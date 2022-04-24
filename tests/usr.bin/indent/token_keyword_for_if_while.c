/* $NetBSD: token_keyword_for_if_while.c,v 1.3 2022/04/24 09:04:12 rillig Exp $ */

/*
 * Tests for the keywords 'for', 'if' and 'while'.  These keywords have in
 * common that they are followed by a space and a parenthesized statement
 * head.  For 'if' and 'while', this head is a single expression.  For 'for',
 * the head is 0 to 3 expressions, separated by semicolons.
 */

//indent input
void
function(void)
{
	if(cond)stmt();
	while(cond)stmt();
	for(;cond;)stmt();
	do stmt();while(cond);
}
//indent end

//indent run
void
function(void)
{
	if (cond)
		stmt();
	while (cond)
		stmt();
	for (; cond;)
		stmt();
	do
		stmt();
	while (cond);
}
//indent end
