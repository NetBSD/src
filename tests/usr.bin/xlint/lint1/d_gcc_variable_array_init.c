/*	$NetBSD: d_gcc_variable_array_init.c,v 1.3 2021/01/31 14:57:28 rillig Exp $	*/
# 3 "d_gcc_variable_array_init.c"

/* gcc: variable array initializer */
void
foo(int i)
{
	int array[i];
	while (i--)
		foo(array[i] = 0);
}
