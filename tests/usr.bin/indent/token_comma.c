/* $NetBSD: token_comma.c,v 1.1 2021/10/18 22:30:34 rillig Exp $ */
/* $FreeBSD$ */

/*
 * Tests for the comma, which is used in the following contexts:
 *
 * The binary operator ',' inserts a sequence point between the evaluation of
 * its operands.
 *
 * The parameters of a function declaration or a macro definition are
 * separated by a comma.
 *
 * The arguments of a function call expression or a macro invocation are
 * separated by a comma.
 */

#indent input
int
comma_expression(void)
{
	return 1,3;
	return a=b,c=d;
}
#indent end

#indent run
int
comma_expression(void)
{
	return 1, 3;
	return a = b, c = d;
}
#indent end

/*
 * A comma that occurs at the beginning of a line is probably part of an
 * initializer list, placed there for alignment.
 */
#indent input
int
comma_at_beginning_of_line(void)
{
	return 1,
	3;
	return 1
	,3;
}
#indent end

#indent run -ci4
int
comma_at_beginning_of_line(void)
{
	return 1,
	    3;
	return 1
	    ,3;
}
#indent end
