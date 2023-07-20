/*	$NetBSD: msg_320.c,v 1.5 2023/07/09 11:01:27 rillig Exp $	*/
# 3 "msg_320.c"

// Test for message: '({ ... })' is a GCC extension [320]

/* lint1-flags: -Sw -X 351 */

int
example(void)
{
	return ({
		int base = 10;
		int square = base * base;
		square * base;
	});
	/* expect-1: warning: '({ ... })' is a GCC extension [320] */
}
