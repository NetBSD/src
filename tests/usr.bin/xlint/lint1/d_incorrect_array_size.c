/*	$NetBSD: d_incorrect_array_size.c,v 1.3 2021/02/21 09:07:58 rillig Exp $	*/
# 3 "d_incorrect_array_size.c"

struct foo {
	int a[-1];		/* expect: 20 */
};
