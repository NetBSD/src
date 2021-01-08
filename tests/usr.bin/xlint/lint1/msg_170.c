/*	$NetBSD: msg_170.c,v 1.2 2021/01/08 21:25:03 rillig Exp $	*/
# 3 "msg_170.c"

// Test for message: first operand must have scalar type, op ? : [170]

struct number {
	int value;
};

_Bool
example(const struct number *num)
{
	return *num ? 1 : 0;
}
