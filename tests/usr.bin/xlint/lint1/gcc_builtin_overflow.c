/*	$NetBSD: gcc_builtin_overflow.c,v 1.2 2021/09/03 22:48:49 rillig Exp $	*/
# 3 "gcc_builtin_overflow.c"

/*
 * Some GCC builtin functions return bool, and in lint's strict bool mode,
 * that makes a difference.
 *
 * https://gcc.gnu.org/onlinedocs/gcc/Integer-Overflow-Builtins.html
 */

/* lint1-extra-flags: -T */

void
is_overflow(void)
{
	int sum;

	if (__builtin_add_overflow(1, 2, &sum))
		return;

	if (__builtin_add_overflow_p(1, 2, 12345))
		return;

	/* expect+1: error: controlling expression must be bool, not 'int' [333] */
	if (__builtin_other(1, 2, 12345))
		return;
}
