/*	$NetBSD: msg_170.c,v 1.6 2023/07/07 19:45:22 rillig Exp $	*/
# 3 "msg_170.c"

// Test for message: first operand must have scalar type, op ? : [170]

/* lint1-extra-flags: -X 351 */

struct number {
	int value;
};

_Bool
/* expect+1: warning: argument 'num' unused in function 'example' [231] */
example(const struct number *num)
{
	/* expect+2: error: first operand must have scalar type, op ? : [170] */
	/* expect+1: warning: function 'example' expects to return value [214] */
	return *num ? 1 : 0;
}
