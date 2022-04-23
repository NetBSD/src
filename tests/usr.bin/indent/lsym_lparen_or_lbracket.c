/* $NetBSD: lsym_lparen_or_lbracket.c,v 1.5 2022/04/23 17:25:58 rillig Exp $ */

/*
 * Tests for the token lsym_lparen_or_lbracket, which represents a '(' or '['
 * in these contexts:
 *
 * In a type name, '(' constructs a function type.
 *
 * In an expression, '(' starts an inner expression to override the usual
 * operator precedence.
 *
 * In an expression, an identifier followed by '(' starts a function call
 * expression.
 *
 * In a 'sizeof' expression, '(' is required if the argument is a type name.
 *
 * After one of the keywords 'for', 'if', 'switch' or 'while', the controlling
 * expression must be enclosed in '(' and ')'; see lsym_for.c, lsym_if.c,
 * lsym_switch.c, lsym_while.c.
 *
 * In an expression, '(' followed by a type name starts a cast expression or
 * a compound literal.
 *
 * In a declaration, '[' derives an array type.
 *
 * In an expression, '[' starts an array subscript.
 */

/* The '(' in a type name derives a function type. */
#indent input
typedef void signal_handler(int);
void (*signal(void (*)(int)))(int);
#indent end

#indent run
typedef void signal_handler(int);
void		(*signal(void (*)(int)))(int);
#indent end


/*
 * The '(' in an expression overrides operator precedence.  In multi-line
 * expressions, the continuation lines are aligned on the parentheses.
 */
#indent input
int nested = (
	(
		(
			(
				1 + 4
			)
		)
	)
);
#indent end

#indent run
int		nested = (
			  (
			   (
			    (
			     1 + 4
			     )
			    )
			   )
);
#indent end


/* The '(' in a function call expression starts the argument list. */
#indent input
int var = macro_call ( arg1,  arg2  ,arg3);
#indent end

#indent run
int		var = macro_call(arg1, arg2, arg3);
#indent end


/*
 * The '(' in a sizeof expression is required for type names and optional for
 * expressions.
 */
#indent input
size_t sizeof_typename = sizeof ( int );
size_t sizeof_expr = sizeof ( 12345 ) ;
#indent end

#indent run
size_t		sizeof_typename = sizeof(int);
size_t		sizeof_expr = sizeof(12345);
#indent end


/* The '[' in a type name derives an array type. */
#indent input
int array_of_numbers[100];
#indent end

#indent run
int		array_of_numbers[100];
#indent end


/* The '[' in an expression accesses an array element. */
#indent input
int second_prime = &primes[1];
#indent end

#indent run
int		second_prime = &primes[1];
#indent end


#indent input
void
function(void)
{
	/* Type casts */
	a = (int)b;
	a = (struct tag)b;
	/* TODO: The '(int)' is not a type cast, it is a prototype list. */
	a = (int (*)(int))fn;

	/* Not type casts */
	a = sizeof(int) * 2;
	a = sizeof(5) * 2;
	a = offsetof(struct stat, st_mtime);

	/* Grouping subexpressions */
	a = ((((b + c)))) * d;
}
#indent end

#indent run-equals-input

/* See t_errors.sh, test case 'compound_literal'. */
