/*	$NetBSD: d_bltinoffsetof.c,v 1.2 2021/01/31 14:39:31 rillig Exp $	*/
# 3 "d_bltinoffsetof.c"

struct foo {
	int a;
	char *b;
};


int
main(void)
{
	return __builtin_offsetof(struct foo, b);
}
