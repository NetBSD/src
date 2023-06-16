/* $NetBSD: lsym_type.c,v 1.1 2023/06/16 23:51:32 rillig Exp $ */

/*
 * Tests for the token lsym_type, which represents a type name in the following
 * contexts:
 *
 * In a declaration that is not for a function.
 *
 * As part of a parameter list of a function prototype.
 *
 * In a cast expression.
 *
 * In a compound expression (since C99).
 *
 * See also:
 *	fmt_decl
 *	lex_ident
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


//indent input
{
{}
size_t hello;
}
//indent end

//indent run
{
	{
	}
	size_t		hello;
}
//indent end


/*
 * In a sizeof expression, a type argument must be enclosed in parentheses.
 */
//indent input
int sizeof_int = sizeof int;
//indent end

//indent run-equals-input -di0
