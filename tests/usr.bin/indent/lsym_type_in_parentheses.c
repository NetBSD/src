/* $NetBSD: lsym_type_in_parentheses.c,v 1.1 2021/11/18 21:19:19 rillig Exp $ */
/* $FreeBSD$ */

/*
 * Tests for the token lsym_token_in_parentheses, which represents a type name
 * inside parentheses in the following contexts:
 *
 * As part of a parameter list of a function prototype.
 *
 * In a cast expression.
 *
 * In a compound expression (since C99).
 */

#indent input
// TODO: add input
#indent end

#indent run-equals-input
