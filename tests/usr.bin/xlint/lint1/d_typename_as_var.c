/*	$NetBSD: d_typename_as_var.c,v 1.2 2021/01/31 14:39:31 rillig Exp $	*/
# 3 "d_typename_as_var.c"

typedef char h[10];

typedef struct {
	int i;
	char *c;
} fh;

struct foo {
	fh h;
	struct {
		int x;
		int y;
	} fl;
};
