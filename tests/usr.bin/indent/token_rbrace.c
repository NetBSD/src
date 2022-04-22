/* $NetBSD: token_rbrace.c,v 1.2 2022/04/22 21:21:20 rillig Exp $ */

/*
 * Tests for the token '}', which ends the corresponding '{' token.
 */

#indent input
void function(void){}
#indent end

#indent run
void
function(void)
{
}
#indent end
