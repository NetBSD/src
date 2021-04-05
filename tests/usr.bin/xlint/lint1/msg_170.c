/*	$NetBSD: msg_170.c,v 1.4 2021/04/05 01:35:34 rillig Exp $	*/
# 3 "msg_170.c"

// Test for message: first operand must have scalar type, op ? : [170]

struct number {
	int value;
};

_Bool
example(const struct number *num)	/* expect: 231 */
{
	return *num ? 1 : 0;		/* expect: 170 *//* expect: 214 */
}
