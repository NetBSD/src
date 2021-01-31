/*	$NetBSD: d_c99_recursive_init.c,v 1.4 2021/01/31 14:57:28 rillig Exp $	*/
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
	{ .s = "foo", .c = 'b', .u = { .uc = 'c' }},
	{ .i = 1, .c = 'a', .u = { .us = 2 }},
};
