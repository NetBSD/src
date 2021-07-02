/*	$NetBSD: c99_init_array.c,v 1.2 2021/07/02 22:46:43 rillig Exp $	*/
# 3 "c99_init_array.c"

/*
 * Test C99-style initialization of arrays.
 */

/* lint1-extra-flags: -p */

// The size of the array is determined by the maximum index, not by the last
// one mentioned.
int arr_11[] = { [10] = 10, [0] = 0 };
typedef int ctassert_11[sizeof(arr_11) / sizeof(arr_11[0]) == 11 ? -1 : 1];
/* expect-1: error: negative array dimension (-1) [20] */

// Without an explicit subscript designator, the subscript counts up.
int arr_3[] = { [1] = 1, [0] = 0, 1, 2 };
typedef int ctassert_3[sizeof(arr_3) / sizeof(arr_3[0]) == 3 ? -1 : 1];
/* expect-1: error: negative array dimension (-1) [20] */
