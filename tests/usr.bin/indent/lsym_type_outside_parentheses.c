/* $NetBSD: lsym_type_outside_parentheses.c,v 1.4 2023/05/15 17:51:49 rillig Exp $ */

/*
 * Tests for the token lsym_type_outside_parentheses, which represents a type
 * name outside parentheses, such as in a declaration that is not for a
 * function.
 *
 * See also:
 *	lex_ident
 *	lsym_type_in_parentheses
 *	lsym_word
 *	opt_ta
 *	opt_T
 */

/*
 * Indent has to guess which identifiers are types and which are variables.
 */
//indent input
t1		       *no_init_ptr;
t2		       *init_ptr = 0;
/* $ FIXME: Assume that an identifier after 'const' is a type name. */
const			t3 * const_no_init_ptr;
static t4	       *static_no_init_ptr;
/* $ FIXME: Assume that an identifier after 'typedef' is a type name. */
typedef t5 * typedef_no_init_ptr;
//indent end

//indent run-equals-input -di24
