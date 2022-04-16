/*	$NetBSD: msg_340.c,v 1.3 2022/04/16 13:25:27 rillig Exp $	*/
# 3 "msg_340.c"

// Test for message: initialization with '[a...b]' is a GCC extension [340]

/*
 * In strict C mode, GCC extensions are flagged as such.
 */

/* lint1-flags: -Sw */

int
example(void)
{
	int numbers[] = {
		[2 ... 3] = 12	/* expect: 340 */
	};
	return numbers[0];
}
