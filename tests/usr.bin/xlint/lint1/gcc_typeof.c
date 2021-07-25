/*	$NetBSD: gcc_typeof.c,v 1.3 2021/07/25 15:58:24 rillig Exp $	*/
# 3 "gcc_typeof.c"

/*
 * Tests for the GCC extension 'typeof'.
 *
 * https://gcc.gnu.org/onlinedocs/gcc/Typeof.html
 */

void take_double(typeof(0.0));

void take_function_double_returning_double(
    typeof(0.0)(
	typeof(0.0)
    )
);

void
cast(double(*fn)(double))
{
	take_double(0.0);

	/* expect+1: warning: passing 'pointer to function(double) returning double' to incompatible 'double', arg #1 [155] */
	take_double(fn);

	/* expect+1: warning: passing 'double' to incompatible 'pointer to function(double) returning double', arg #1 [155] */
	take_function_double_returning_double(0.0);

	take_function_double_returning_double(fn);

	/* identity cast */
	take_function_double_returning_double((double (*)(double))fn);
}
