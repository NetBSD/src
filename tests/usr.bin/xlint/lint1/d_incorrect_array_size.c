/*	$NetBSD: d_incorrect_array_size.c,v 1.2 2021/01/31 14:39:31 rillig Exp $	*/
# 3 "d_incorrect_array_size.c"

struct foo {
	int a[-1];
};
