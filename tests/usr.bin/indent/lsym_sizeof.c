/* $NetBSD: lsym_sizeof.c,v 1.4 2022/04/22 21:21:20 rillig Exp $ */

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
 */
#indent input
char str[sizeof(int) * CHAR_BIT + 1];
#indent end

#indent run-equals-input -di0
