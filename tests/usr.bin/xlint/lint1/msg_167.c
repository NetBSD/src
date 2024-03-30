/*	$NetBSD: msg_167.c,v 1.7 2024/03/30 16:47:45 rillig Exp $	*/
# 3 "msg_167.c"

// Test for message: array subscript %jd cannot be negative [167]

/* lint1-extra-flags: -X 351 */

void
example(int *ptr)
{
	int arr[6];

	/* expect+1: warning: array subscript -3 cannot be negative [167] */
	arr[-3] = 13;

	/*
	 * Since the pointer may have been initialized with "arr + 3",
	 * subtracting from its address is allowed.
	 */
	ptr[-3] = 13;
}
