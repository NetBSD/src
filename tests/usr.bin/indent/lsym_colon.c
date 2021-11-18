/* $NetBSD: lsym_colon.c,v 1.1 2021/11/18 21:19:19 rillig Exp $ */
/* $FreeBSD$ */

/*
 * Tests for the token lsym_colon, which represents a ':' in these contexts:
 *
 * After a label that is the target of a 'goto' statement.
 *
 * In a 'switch' statement, after a 'case' label or a 'default' label.
 *
 * As part of the conditional operator '?:'.
 *
 * In the declaration of a struct member that is a bit-field.
 */

#indent input
// TODO: add input
#indent end

#indent run-equals-input
