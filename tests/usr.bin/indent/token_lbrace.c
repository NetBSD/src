/* $NetBSD: token_lbrace.c,v 1.1 2021/10/18 22:30:34 rillig Exp $ */
/* $FreeBSD$ */

/*
 * Tests for the token '{'.
 *
 * It is used as the start marker of a block of statements.
 *
 * It is used in initializers.
 *
 * In macro arguments, a '{' is an ordinary character, it does not need to be
 * balanced.  This is in contrast to '(', which must be balanced with ')'.
 */

#indent input
void
function(void)
{
	struct person	p = {
		.name = "Name",
		.age = {{{35}}},	/* C11 6.7.9 allows this. */
	};
}
#indent end

#indent run-equals-input
