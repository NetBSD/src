/* $NetBSD: lsym_comma.c,v 1.2 2021/11/20 16:54:17 rillig Exp $ */
/* $FreeBSD$ */

/*
 * Tests for the token lsym_comma, which represents a ',' in these contexts:
 *
 * In an expression, the binary operator ',' evaluates its left operand before
 * its right operand, inserting a sequence point.
 *
 * In a declaration, a ',' separates the declarators.
 *
 * In a parameter list of a function type, a ',' separates the parameter
 * declarations.
 *
 * In a traditional function definition, a ',' separates the parameter names.
 *
 * In a prototype function definition, a ',' separates the parameter
 * declarations.
 *
 * In a function call expression, a ',' separates the arguments.
 *
 * In a macro definition, a ',' separates the parameter names.
 *
 * In a macro invocation, a ',' separates the arguments.
 */

#indent input
// TODO: add input
#indent end

#indent run-equals-input
