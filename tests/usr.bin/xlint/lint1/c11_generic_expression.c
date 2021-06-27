/*	$NetBSD: c11_generic_expression.c,v 1.3 2021/06/27 20:47:13 rillig Exp $	*/
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
	/* expect-2: argument 'var' unused */

	return _Generic(var,
	    long double: "long double",
	    long long: "long long",
	    unsigned: "unsigned"
	);
	/* expect-1: expects to return value [214] */
}

/*
 * In this case, the 'default' expression is selected.
 */
const char *
classify_type_with_default(double var)
{
	/* expect-2: argument 'var' unused */

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
	/* expect-2: argument 'c' unused */

	return _Generic(c,
	    char: "yes",
	    default: 0.0
	);
}
