/*	$NetBSD: d_c99_recursive_init.c,v 1.5 2021/02/20 22:31:20 rillig Exp $	*/
# 3 "d_c99_recursive_init.c"

/* C99 recursive struct/union initialization */
struct top {
	int i;
	char c;
	union onion {
		short us;
		char uc;
	} u;
	char *s;
} c[] = {
	{
		.s = "foo",
		.c = 'b',
		.u = {
			.uc = 'c'
		}
	},
	{
		.i = 1,
		.c = 'a',
		.u = {
			.us = 2
		}
	},
};
