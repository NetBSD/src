/*	$NetBSD: msg_173.c,v 1.5 2023/03/28 14:44:35 rillig Exp $	*/
# 3 "msg_173.c"

// Test for message: too many array initializers, expected %d [173]

/* lint1-extra-flags: -X 351 */

/* expect+1: error: too many array initializers, expected 3 [173] */
int arr_limited[3] = { 1, 2, 3, 4 };
int arr_unlimited[] = { 1, 2, 3, 4 };
int arr_too_few[] = { 1, 2 };
