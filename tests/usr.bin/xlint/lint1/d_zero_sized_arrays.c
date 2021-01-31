/*	$NetBSD: d_zero_sized_arrays.c,v 1.2 2021/01/31 14:39:31 rillig Exp $	*/
# 3 "d_zero_sized_arrays.c"

struct foo {
int a[0];
};
