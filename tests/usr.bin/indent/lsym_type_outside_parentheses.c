/* $NetBSD: lsym_type_outside_parentheses.c,v 1.5 2023/05/15 18:22:40 rillig Exp $ */

/*
 * Tests for the token lsym_type_outside_parentheses, which represents a type
 * name outside parentheses, such as in a declaration that is not for a
 * function.
 *
 * See also:
 *	fmt_decl
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
const t3	       *const_no_init_ptr;
static t4	       *static_no_init_ptr;
typedef t5 *typedef_no_init_ptr;

// $ XXX: There's no point aligning the word 'const' with the other names.
const char	       *const names[3];
//indent end

//indent run-equals-input -di24
