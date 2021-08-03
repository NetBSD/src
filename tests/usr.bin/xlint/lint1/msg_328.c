/*	$NetBSD: msg_328.c,v 1.3 2021/08/03 20:57:06 rillig Exp $	*/
# 3 "msg_328.c"

// Test for message: union cast is a GCC extension [328]

/* lint1-flags: -Sw */

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
