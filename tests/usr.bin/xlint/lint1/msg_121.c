/*	$NetBSD: msg_121.c,v 1.4 2021/04/06 21:44:12 rillig Exp $	*/
# 3 "msg_121.c"

// Test for message: negative shift [121]

int
example(int x)
{
	return x << (3 - 5);		/* expect: 121 */
}

void /*ARGSUSED*/
shift_by_double(int x, double amount)
{
	/*
	 * This is already caught by typeok_scalar, so it doesn't reach
	 * typeok_shift via typeok_op.
	 */
	return x << amount;		/* expect: incompatible types */
}
