/* $NetBSD: lsym_semicolon.c,v 1.3 2022/04/24 09:04:12 rillig Exp $ */

/*
 * Tests for the token lsym_semicolon, which represents ';' in these contexts:
 *
 * In a declaration, ';' finishes the declaration.
 *
 * In a statement, ';' finishes the statement.
 *
 * In a 'for' statement, ';' separates the 3 expressions in the head of the
 * 'for' statement.
 */

//indent input
// TODO: add input
//indent end

//indent run-equals-input
