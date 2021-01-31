/*	$NetBSD: d_c9x_recursive_init.c,v 1.2 2021/01/31 14:39:31 rillig Exp $	*/
# 3 "d_c9x_recursive_init.c"

/* C9X struct/union member init, with nested union and trailing member */
union node {
	void *next;
	char *data;
};
struct foo {
	int b;
	union node n;
	int c;
};

struct foo f = {
	.b = 1,
	.n = { .next = 0, },
	.c = 1
};
