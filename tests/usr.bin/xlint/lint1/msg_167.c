/*	$NetBSD: msg_167.c,v 1.6 2024/03/27 19:28:20 rillig Exp $	*/
# 3 "msg_167.c"

// Test for message: array subscript cannot be negative: %jd [167]

/* lint1-extra-flags: -X 351 */

void
example(int *ptr)
{
	int arr[6];

	/* expect+1: warning: array subscript cannot be negative: -3 [167] */
	arr[-3] = 13;

	/*
	 * Since the pointer may have been initialized with "arr + 3",
	 * subtracting from its address is allowed.
	 */
	ptr[-3] = 13;
}
