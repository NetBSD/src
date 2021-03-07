/*	$NetBSD: msg_340.c,v 1.1 2021/03/07 19:42:54 rillig Exp $	*/
# 3 "msg_340.c"

// Test for message: initialization with '[a...b]' is a GNU extension [340]

/*
 * In strict C mode, GNU extensions are flagged as such.
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
