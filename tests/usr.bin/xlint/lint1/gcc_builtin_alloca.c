/*	$NetBSD: gcc_builtin_alloca.c,v 1.1 2021/12/06 23:20:26 rillig Exp $	*/
# 3 "gcc_builtin_alloca.c"

/*
 * Test for the GCC builtin functions __builtin_alloca*, which unlike most
 * other builtin functions return a pointer instead of int.
 *
 * https://gcc.gnu.org/onlinedocs/gcc/Other-Builtins.html
 */

void
example(void)
{
	/* expect+1: warning: illegal combination of pointer (pointer to char) and integer (int) [183] */
	char *ptr = __builtin_alloca(8);
	ptr[4] = '4';

	/* expect+1: warning: illegal combination of pointer (pointer to char) and integer (int) [183] */
	char *aligned_ptr = __builtin_alloca_with_align(8, 64);
	aligned_ptr[0] = '\0';
}
