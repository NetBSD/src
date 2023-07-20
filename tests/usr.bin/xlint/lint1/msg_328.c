/*	$NetBSD: msg_328.c,v 1.4 2023/07/07 19:45:22 rillig Exp $	*/
# 3 "msg_328.c"

// Test for message: union cast is a GCC extension [328]

/* lint1-flags: -Sw -X 351 */

union target {
	int b;
};

void
foo(void)
{
	union target arg = { 123 };
	/* expect+1: error: union cast is a GCC extension [328] */
	arg = (union target)3;
	arg.b++;
}
