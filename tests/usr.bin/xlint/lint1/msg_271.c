/*	$NetBSD: msg_271.c,v 1.4 2021/08/22 13:52:19 rillig Exp $	*/
# 3 "msg_271.c"

/* Test for message: switch expression must be of type 'int' in traditional C [271] */

/* lint1-flags: -tw */

example(long_int, unsigned_int)
	long long_int;
	unsigned unsigned_int;
{
	/* expect+1: warning: switch expression must be of type 'int' in traditional C [271] */
	switch (long_int) {
	case 3:
		return 1;
	}

	/*
	 * XXX: K&R clearly says "the result must be 'int'", but lint also
	 * allows unsigned int.
	 */
	switch (unsigned_int) {
	case 3:
		return 1;
	}
	return 2;
}
