/*	$NetBSD: d_c99_for_loops.c,v 1.3 2021/01/31 14:39:31 rillig Exp $	*/
# 3 "d_c99_for_loops.c"

/* c99 for loops */
extern void foo(int);

int
main(void)
{
	// Test the basic functionality
	for (int i = 0; i < 10; i++)
		foo(i);

	// Test that the scope of the iterator is correct
	for (int i = 0; i < 10; i++)
		continue;
	return 0;
}
