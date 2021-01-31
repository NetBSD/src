/*	$NetBSD: msg_170.c,v 1.3 2021/01/31 11:12:07 rillig Exp $	*/
# 3 "msg_170.c"

// Test for message: first operand must have scalar type, op ? : [170]

struct number {
	int value;
};

_Bool
example(const struct number *num)	/* expect: 231 */
{
	return *num ? 1 : 0;		/* expect: 170, 214 */
}
