/*	$NetBSD: d_zero_sized_arrays.c,v 1.3 2021/01/31 14:57:28 rillig Exp $	*/
# 3 "d_zero_sized_arrays.c"

struct foo {
	int a[0];
};
