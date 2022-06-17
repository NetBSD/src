/*	$NetBSD: msg_340.c,v 1.4 2022/06/17 06:59:16 rillig Exp $	*/
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
		/* expect+1: warning: initialization with '[a...b]' is a GCC extension [340] */
		[2 ... 3] = 12
	};
	return numbers[0];
}
