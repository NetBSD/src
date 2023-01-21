/*	$NetBSD: gcc_typeof.c,v 1.5 2023/01/21 08:04:43 rillig Exp $	*/
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

/*
 * Since cgram 1.58 from 2014-02-18, when support for __typeof__ was added,
 * and before cgram.y 1.394 from 2022-04-10, lint ran into an assertion
 * failure when encountering a redundant type qualifier 'const' or 'volatile'
 * in a declaration using __typeof__.  Seen in sys/sys/lwp.h on x86_64, in
 * the call atomic_load_consume(&l->l_mutex).
 */
int *volatile lock;
const volatile __typeof__(lock) *lock_pointer = &lock;

/*
 * Before cgram.y 1.427 from 2023-01-21, lint crashed due to a null pointer
 * dereference if the __typeof__ operator had an invalid argument.  Seen in
 * _fc_atomic_ptr_cmpexch from fontconfig, which uses <stdatomic.h> provided
 * by GCC, which in turn uses __auto_type and __typeof__, and lint doesn't
 * know about __auto_type.
 */
void _fc_atomic_ptr_cmpexch(void)
{
	/* expect+1: error: 'expr' undefined [99] */
	__typeof__ (expr) var = 0;
}
