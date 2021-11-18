/* $NetBSD: lsym_period.c,v 1.1 2021/11/18 21:19:19 rillig Exp $ */
/* $FreeBSD$ */

/*
 * Tests for the token lsym_period, which represents '.' in these contexts:
 *
 * In an initializer, '.' starts a named designator (since C99).
 *
 * In an expression, 'sou.member' accesses the member 'member' in the struct
 * or union 'sou'.
 *
 * In a function prototype declaration, the sequence '.' '.' '.' marks the
 * start of a variable number of arguments.
 *
 * See also:
 *	lsym_word.c		for '.' inside numeric constants
 */

#indent input
// TODO: add input
#indent end

#indent run-equals-input
