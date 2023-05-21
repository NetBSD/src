/* $NetBSD: lsym_type_in_parentheses.c,v 1.4 2023/05/21 10:18:44 rillig Exp $ */

/*
 * Tests for the token lsym_type_in_parentheses, which represents a type name
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
