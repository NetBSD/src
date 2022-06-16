/*	$NetBSD: msg_173.c,v 1.4 2022/06/16 16:58:36 rillig Exp $	*/
# 3 "msg_173.c"

// Test for message: too many array initializers, expected %d [173]

/* expect+1: error: too many array initializers, expected 3 [173] */
int arr_limited[3] = { 1, 2, 3, 4 };
int arr_unlimited[] = { 1, 2, 3, 4 };
int arr_too_few[] = { 1, 2 };
