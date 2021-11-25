/* $NetBSD: lsym_sizeof.c,v 1.2 2021/11/25 18:10:23 rillig Exp $ */
/* $FreeBSD$ */

/*
 * Tests for the token lsym_sizeof, which represents the keyword 'sizeof' for
 * determining the memory size of an object or a type.
 *
 * See also:
 *	opt_bs.c		"blank after sizeof"
 *	C11 6.5.3.4		"The 'sizeof' and '_Alignof' operators"
 */

#indent input
// TODO: add input
#indent end

#indent run-equals-input


/*
 * After 'sizeof', a type name in parentheses does not start a cast
 * expression.
 *
 * Broken since lexi.c 1.156 from 2021-11-25.
 */
#indent input
char str[sizeof(int) * CHAR_BIT + 1];
#indent end

/* FIXME: The '*' must be a binary operator here. */
#indent run -di0
char str[sizeof(int) *CHAR_BIT + 1];
#indent end
