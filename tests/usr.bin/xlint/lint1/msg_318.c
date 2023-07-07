/*	$NetBSD: msg_318.c,v 1.4 2023/07/07 19:45:22 rillig Exp $	*/
# 3 "msg_318.c"

// Test for message: variable array dimension is a C99/GCC extension [318]

/* lint1-flags: -Sw -X 351 */

unsigned long
example(int n)
{
	unsigned long arr[n];

	arr[n - 1] = '\0';
	arr[0] = sizeof(unsigned long[n]);
	return arr[n - 1];
}

/* expect+1: error: syntax error ':' [249] */
TODO: "Add example code that triggers the above message."
TODO: "Add example code that almost triggers the above message."
