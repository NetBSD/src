/*	$NetBSD: c11_generic_expression.c,v 1.23 2026/01/10 08:42:48 rillig Exp $	*/
# 3 "c11_generic_expression.c"

/* lint1-extra-flags: -X 351 */

/*
 * C99 added support for type-generic macros, but these were limited to the
 * header <tgmath.h>.  C11 made this feature generally available.
 *
 * The generic selection is typically used with macros, but since lint1 works
 * on the preprocessed source, the test cases look a bit strange.
 *
 * C11 6.5.1.1 "Generic selection"
 */

/* lint1-extra-flags: -Ac11 */

/*
 * The type of 'var' is not compatible with any of the types from the
 * generic-association.  This is a constraint violation that the compiler must
 * detect, therefore lint doesn't repeat that diagnostic.
 */
const char *
classify_type_without_default(double var)
{
	/* expect-2: warning: parameter 'var' unused in function 'classify_type_without_default' [231] */

	return _Generic(var,
	    long double: "long double",
	    long long: "long long",
	    unsigned: "unsigned"
	);
	/* expect-1: error: function 'classify_type_without_default' expects to return value [214] */
}

/*
 * In this case, the 'default' expression is selected.
 */
const char *
classify_type_with_default(double var)
{
	/* expect-2: warning: parameter 'var' unused in function 'classify_type_with_default' [231] */

	return _Generic(var,
	    long double: "long double",
	    long long: "long long",
	    unsigned: "unsigned",
	    default: "unknown"
	);
}

/*
 * The type of a _Generic expression is the one from the selected association.
 */
const char *
classify_char(char c)
{
	/* expect-2: warning: parameter 'c' unused in function 'classify_char' [231] */

	return _Generic(c,
	    char: "yes",
	    default: 0.0
	);
}

/*
 * Before cgram.y 1.238 from 2021-06-27, lint accepted a comma-expression,
 * which looked as if _Generic would accept multiple arguments before the
 * selection.
 */
/* ARGSUSED */
const int *
comma_expression(char first, double second)
{
	/* expect+1: error: syntax error 'second' [249] */
	return _Generic(first, second,
	    char: "first",
	    double: 2.0
	);
	/* expect+1: warning: function 'comma_expression' falls off bottom without returning value [217] */
}

/*
 * Ensure that assignment-expressions are accepted by the grammar, as
 * opposed to comma-expressions.
 */
/* ARGSUSED */
int
assignment_expression(int first, int second)
{
	return _Generic(first = second,
	    int: second = first
	);
}

int
primary_expression(void)
{
	return _Generic(0, int: assignment_expression)(0, 0);
}

/*
 * The types don't match, therefore build_generic_selection returns NULL,
 * which is then silently ignored by init_expr.  This situation is already
 * covered by the compilers, so there is no need for lint to double-check it.
 */
const char *x = _Generic(
    1ULL + 1.0f,
    int: 1
);

// Introduced by C11 6.5.2.1.
// Fixed by C23 6.5.2.1.
void
type_conversions(const char *str)
{
	int array[4] = { 0, 1, 2, 3 };

	// Type qualifiers are removed in lvalues.
	x = _Generic(*str,
		char: str,
		default: primary_expression
	);
	// Type qualifiers are ignored in rvalues.
	x = _Generic(((const unsigned char)str[2]),
		unsigned char: str
	);
	// Functions are converted to a function pointer.
	x = _Generic(type_conversions,
		void(*)(const char *): str,
		default: primary_expression
	);
	// Arrays are converted to the corresponding pointer.
	x = _Generic(array,
		int *: str,
		default: primary_expression
	);
}
