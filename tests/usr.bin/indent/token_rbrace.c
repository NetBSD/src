/* $NetBSD: token_rbrace.c,v 1.1 2021/10/18 22:30:34 rillig Exp $ */
/* $FreeBSD$ */

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
