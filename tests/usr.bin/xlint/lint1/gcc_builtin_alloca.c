/*	$NetBSD: gcc_builtin_alloca.c,v 1.4 2023/07/07 19:45:22 rillig Exp $	*/
# 3 "gcc_builtin_alloca.c"

/*
 * Test for the GCC builtin functions __builtin_alloca*, which unlike most
 * other builtin functions return a pointer instead of int.
 *
 * https://gcc.gnu.org/onlinedocs/gcc/Other-Builtins.html
 */

/* lint1-extra-flags: -X 351 */

void
example(void)
{
	char *ptr = __builtin_alloca(8);
	ptr[4] = '4';

	char *aligned_ptr = __builtin_alloca_with_align(8, 64);
	aligned_ptr[0] = '\0';

	/* expect+1: warning: illegal combination of pointer 'pointer to char' and integer 'int' [183] */
	char *unknown = __builtin_allocate(8);
	unknown[0] = '\0';
}
