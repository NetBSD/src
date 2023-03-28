/*	$NetBSD: d_c99_for_loops.c,v 1.4 2023/03/28 14:44:34 rillig Exp $	*/
# 3 "d_c99_for_loops.c"

/* lint1-extra-flags: -X 351 */

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
