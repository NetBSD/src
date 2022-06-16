/*	$NetBSD: msg_167.c,v 1.4 2022/06/16 16:58:36 rillig Exp $	*/
# 3 "msg_167.c"

// Test for message: array subscript cannot be negative: %ld [167]

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
