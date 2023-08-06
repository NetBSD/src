/*	$NetBSD: msg_170.c,v 1.8 2023/08/06 19:44:50 rillig Exp $	*/
# 3 "msg_170.c"

// Test for message: first operand of '?' must have scalar type [170]

/* lint1-extra-flags: -X 351 */

struct number {
	int value;
};

_Bool
/* expect+1: warning: parameter 'num' unused in function 'example' [231] */
example(const struct number *num)
{
	/* expect+2: error: first operand of '?' must have scalar type [170] */
	/* expect+1: error: function 'example' expects to return value [214] */
	return *num ? 1 : 0;
}
