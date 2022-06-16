/*	$NetBSD: msg_170.c,v 1.5 2022/06/16 16:58:36 rillig Exp $	*/
# 3 "msg_170.c"

// Test for message: first operand must have scalar type, op ? : [170]

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
