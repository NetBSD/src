/*	$NetBSD: msg_320.c,v 1.3 2022/06/17 06:59:16 rillig Exp $	*/
# 3 "msg_320.c"

// Test for message: ({ }) is a GCC extension [320]

/* lint1-flags: -Sw */

int
example(void)
{
	return ({
		int base = 10;
		int square = base * base;
		square * base;
	});
	/* expect-1: warning: ({ }) is a GCC extension [320] */
}
