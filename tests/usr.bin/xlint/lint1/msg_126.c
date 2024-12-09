/*	$NetBSD: msg_126.c,v 1.9 2024/12/09 22:43:33 rillig Exp $	*/
# 3 "msg_126.c"

// Test for message: incompatible types '%s' and '%s' in conditional [126]

/* lint1-extra-flags: -X 351 -q 1 */

/* ARGSUSED */
void
example(int i, void *ptr, double dbl, void (*return_void)(void))
{
	/* expect+1: error: incompatible types 'pointer to void' and 'double' in conditional [126] */
	i = i > 0 ? ptr : dbl;

	ptr = i > 0 ? ptr : (void *)0;

	ptr = i > 0 ? ptr : 0;

	/* expect+1: implicit conversion from floating point 'double' to integer 'int' [Q1] */
	i = i > 0 ? dbl : i;

	// GCC accepts the 'int/void' mismatch even with -Wall -Wextra -std=c99.
	/* expect+1: warning: incompatible types 'void' and 'int' in conditional [126] */
	i > 0 ? return_void() : 0;

	i > 0 ? return_void() : (void)0;
}
