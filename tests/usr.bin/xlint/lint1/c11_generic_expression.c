/*	$NetBSD: c11_generic_expression.c,v 1.2 2021/06/27 19:59:23 rillig Exp $	*/
# 3 "c11_generic_expression.c"

/*
 * C99 added support for type-generic macros, but these were limited to the
 * header <tgmath.h>.  C11 made this feature generally available.
 *
 * The generic selection is typically used with macros, but since lint1 works
 * on the preprocessed source, the test cases look a bit strange.
 *
 * C99 6.5.1.1 "Generic selection"
 */

/* lint1-extra-flags: -Ac11 */

/*
 * The type of 'var' is not compatible with any of the types from the
 * generic-association.  This is a compile-time error.
 */
const char *
classify_type_without_default(double var)
{
	return _Generic(var,
	    long double: "long double",
	    long long: "long long",
	    unsigned: "unsigned"
	);
	/* expect-7: argument 'var' unused */
	/* expect-2: type mismatch (pointer to const char) and (double) *//* FIXME */
}

/*
 * In this case, the 'default' expression is selected.
 */
const char *
classify_type_with_default(double var)
{
	return _Generic(var,
	    long double: "long double",
	    long long: "long long",
	    unsigned: "unsigned",
	    default: "unknown"
	);
	/* expect-8: argument 'var' unused */
	/* expect-2: type mismatch (pointer to const char) and (double) *//* FIXME */
}

/*
 * The type of a _Generic expression is the one from the selected association.
 */
const char *
classify_char(char c)
{
	return _Generic(c,
	    char: "yes",
	    default: 0.0
	);
	/* expect-1: (pointer to const char) and integer (char) [183] */
}
