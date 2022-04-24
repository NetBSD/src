/* $NetBSD: lsym_comma.c,v 1.5 2022/04/24 09:04:12 rillig Exp $ */

/*
 * Tests for the token lsym_comma, which represents a ',' in these contexts:
 *
 * In an expression, the binary operator ',' evaluates its left operand before
 * its right operand, inserting a sequence point.
 *
 * In a declaration, a ',' separates the declarators.
 *
 * In a parameter list of a function type, a ',' separates the parameter
 * declarations.
 *
 * In a traditional function definition, a ',' separates the parameter names.
 *
 * In a prototype function definition, a ',' separates the parameter
 * declarations.
 *
 * In a function call expression, a ',' separates the arguments.
 *
 * In a macro definition, a ',' separates the parameter names.
 *
 * In a macro invocation, a ',' separates the arguments.
 *
 * In an initializer list, a ',' separates the initializer expressions.
 */

/*
 * The ',' is a binary operator with very low precedence.
 */
//indent input
int
comma_expression(void)
{
	return 1, 3;
	return a = b, c = d;
}
//indent end

//indent run-equals-input


/*
 * In a declaration, a ',' separates the declarators.
 */
//indent input
int decl, old_style(), prototype(const char *, double *);
int a, b, c;
//indent end

//indent run-equals-input -di0


/*
 * In a parameter list of a function type, a ',' separates the parameter
 * declarations.
 */
//indent input
double dbl_reduce(double init, const double *s, const double *e, double (*merge)(double, double));
double dbl_reduce(double, const double *, const double *, double (*)(double, double));
void debug_printf(const char *, ...);
//indent end

//indent run-equals-input -di0


/*
 * In a traditional function definition, a ',' separates the parameter names.
 */
//indent input
double
trad_dbl_reduce(init, s, e, merge)
	double init;
	double *s, *e;
	double (*merge)()
{
	double x = init;
	while (s < e)
		x = merge(x, *s++);
	return x;
}
//indent end

//indent run-equals-input -di0


/*
 * In a prototype function definition, a ',' separates the parameter
 * declarations.
 */
//indent input
void
dbl_reduce(double init, const double *s, const double *e, double (*merge)(double, double))
{
	double x = init;
	while (s < e)
		x = merge(x, *s++);
	return x;
}
//indent end

//indent run-equals-input -di0


/*
 * In a function call expression, a ',' separates the arguments.
 */
//indent input
void
function(void)
{
	function_call(arg1, arg2);
	(*indirect_function_call)(arg1, arg2);
}
//indent end

//indent run-equals-input -di0


/*
 * In a macro definition, a ',' separates the parameter names.
 */
//indent input
#define no_space(a,b) a ## b
#define normal_space(a, b) a ## b
#define wide_space(a  ,  b) a ## b
//indent end

/*
 * Indent does not touch preprocessor directives, except for the spacing
 * between the '#' and the directive.
 */
//indent run-equals-input


/*
 * In a macro invocation, a ',' separates the arguments.
 */
//indent input
void
function(void)
{
	macro_invocation(arg1, arg2);
	empty_arguments(,,,);
}
//indent end

//indent run-equals-input -di0


/*
 * In an initializer list, a ',' separates the initializer expressions.
 *
 * If a ',' starts a line, indent doesn't put a space before it.
 */
//indent input
int arr[] = {1, 2, 3};
int arr[] = {
	1,
	2,
	3,			/* there may be a trailing comma */
};
//indent end

//indent run-equals-input -di0


/*
 * If a ',' starts a line, indent doesn't put a space before it. This style is
 * uncommon and looks unbalanced since the '1' is not aligned to the other
 * numbers.
 */
//indent input
int arr[] = {
	1
	,2
	,3
};
//indent end

//indent run-equals-input -di0
