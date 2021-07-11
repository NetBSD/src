/*	$NetBSD: decl.c,v 1.3 2021/07/11 12:12:30 rillig Exp $	*/
# 3 "decl.c"

/*
 * Tests for declarations, especially the distinction between the
 * declaration-specifiers and the declarators.
 */

/*
 * Even though 'const' comes after 'char' and is therefore quite close to the
 * first identifier, it applies to both identifiers.
 */
void
specifier_qualifier(void)
{
	char const a = 1, b = 2;

	/* expect+1: warning: left operand of '=' must be modifiable lvalue [115] */
	a = 1;
	/* expect+1: warning: left operand of '=' must be modifiable lvalue [115] */
	b = 2;
}

/*
 * Since 'const' comes before 'char', there is no ambiguity whether the
 * 'const' applies to all variables or just to the first.
 */
void
qualifier_specifier(void)
{
	const char a = 1, b = 2;

	/* expect+1: warning: left operand of '=' must be modifiable lvalue [115] */
	a = 3;
	/* expect+1: warning: left operand of '=' must be modifiable lvalue [115] */
	b = 5;
}

void
declarator_with_prefix_qualifier(void)
{
	/* expect+1: syntax error 'const' [249] */
	char a = 1, const b = 2;

	a = 1;
	/* expect+1: error: 'b' undefined [99] */
	b = 2;
}

void
declarator_with_postfix_qualifier(void)
{
	/* expect+1: syntax error 'const' [249] */
	char a = 1, b const = 2;

	a = 1;
	b = 2;
}

void sink(double *);

void
declarators(void)
{
	char *pc = 0, c = 0, **ppc = 0;

	/* expect+1: warning: converting 'pointer to char' to incompatible 'pointer to double' */
	sink(pc);
	/* expect+1: warning: illegal combination of pointer (pointer to double) and integer (char) */
	sink(c);
	/* expect+1: converting 'pointer to pointer to char' to incompatible 'pointer to double' */
	sink(ppc);
}

_Bool
enum_error_handling(void)
{
	enum {
		/* expect+1: syntax error '"' [249] */
		"error 1"
		:		/* still the same error */
		,		/* back on track */
		A,
		B
	} x = A;

	return x == B;
}

void
unused_local_variable(void)
{
	/*FIXME*//* expect+1: syntax error '_Bool' [249] */
	__attribute__((unused)) _Bool unused_var;

	__attribute__((unused))
	/*FIXME*//* expect+2: syntax error '__attribute__' [249] */
	/*FIXME*//* expect+1: cannot recover from previous errors [224] */
	__attribute__((unused)) _Bool unused_twice;
}
