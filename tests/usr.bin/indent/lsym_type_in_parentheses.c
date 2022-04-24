/* $NetBSD: lsym_type_in_parentheses.c,v 1.3 2022/04/24 09:04:12 rillig Exp $ */

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

//indent input
// TODO: add input
//indent end

//indent run-equals-input
