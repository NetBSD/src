/* $NetBSD: lsym_lparen_or_lbracket.c,v 1.2 2022/02/13 11:07:48 rillig Exp $ */
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

#indent input
// TODO: add input
#indent end

#indent run-equals-input

/* See t_errors.sh, test case 'compound_literal'. */
