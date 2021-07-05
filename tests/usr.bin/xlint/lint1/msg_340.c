/*	$NetBSD: msg_340.c,v 1.2 2021/07/05 19:53:43 rillig Exp $	*/
# 3 "msg_340.c"

// Test for message: initialization with '[a...b]' is a GCC extension [340]

/*
 * In strict C mode, GCC extensions are flagged as such.
 */

/* lint1-flags: -Ssw */

int
example(void)
{
	int numbers[] = {
		[2 ... 3] = 12	/* expect: 340 */
	};
	return numbers[0];
}
