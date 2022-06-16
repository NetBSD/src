/*	$NetBSD: msg_121.c,v 1.5 2022/06/16 16:58:36 rillig Exp $	*/
# 3 "msg_121.c"

// Test for message: negative shift [121]

int
example(int x)
{
	/* expect+1: warning: negative shift [121] */
	return x << (3 - 5);
}

void /*ARGSUSED*/
shift_by_double(int x, double amount)
{
	/*
	 * This is already caught by typeok_scalar, so it doesn't reach
	 * typeok_shift via typeok_op.
	 */
	/* expect+1: error: operands of '<<' have incompatible types (int != double) [107] */
	return x << amount;
}
