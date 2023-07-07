/*	$NetBSD: d_gcc_variable_array_init.c,v 1.4 2023/07/07 19:45:22 rillig Exp $	*/
# 3 "d_gcc_variable_array_init.c"

/* gcc: variable array initializer */

/* lint1-extra-flags: -X 351 */

void
foo(int i)
{
	int array[i];
	while (i--)
		foo(array[i] = 0);
}
