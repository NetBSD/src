/*	$NetBSD: gcc_typeof.c,v 1.1 2021/07/25 10:57:38 rillig Exp $	*/
# 3 "gcc_typeof.c"

/*
 * Tests for the GCC extension 'typeof'.
 *
 * https://gcc.gnu.org/onlinedocs/gcc/Typeof.html
 */

void take_double(typeof(0.0));

void take_function_double_returning_double(
    typeof(0.0)(
	/* FIXME: GCC can parse this */
	/* expect+1: error: syntax error 'typeof' [249] */
	typeof(0.0)
    )
);

void
cast(double(*fn)(double))
{
	take_double(0.0);

	/* expect+1: warning: passing 'pointer to function(double) returning double' to incompatible 'double', arg #1 [155] */
	take_double(fn);

	/* XXX: oops; GCC detects this type mismatch. probably due to the parse error. */
	take_function_double_returning_double(0.0);

	take_function_double_returning_double(fn);

	/* identity cast */
	take_function_double_returning_double((double (*)(double))fn);
}
