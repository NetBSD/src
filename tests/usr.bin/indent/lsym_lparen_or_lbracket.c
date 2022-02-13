/* $NetBSD: lsym_lparen_or_lbracket.c,v 1.3 2022/02/13 12:04:37 rillig Exp $ */
/* $FreeBSD$ */

/*
 * Tests for the token lsym_lparen_or_lbracket, which represents a '(' or '['
 * in these contexts:
 *
 * In a type name, '(' constructs a function type.
 *
 * In an expression, '(' starts an inner expression to override the usual
 * operator precedence.
 *
 * In an expression, an identifier followed by '(' starts a function call
 * expression.
 *
 * In a 'sizeof' expression, '(' is required if the argument is a type name.
 *
 * After one of the keywords 'for', 'if', 'switch' or 'while', the controlling
 * expression must be enclosed in '(' and ')'.
 *
 * In an expression, '(' followed by a type name starts a cast expression.
 */

// TODO: Add systematic tests for all cases.

#indent input
void
function(void)
{
	/* Type casts */
	a = (int)b;
	a = (struct tag)b;
	/* TODO: The '(int)' is not a type cast, it is a prototype list. */
	a = (int (*)(int))fn;

	/* Not type casts */
	a = sizeof(int) * 2;
	a = sizeof(5) * 2;
	a = offsetof(struct stat, st_mtime);

	/* Grouping subexpressions */
	a = ((((b + c)))) * d;
}
#indent end

#indent run-equals-input

/* See t_errors.sh, test case 'compound_literal'. */
