/*	$NetBSD: msg_170.c,v 1.7 2023/07/09 11:18:55 rillig Exp $	*/
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
	/* expect+1: warning: function 'example' expects to return value [214] */
	return *num ? 1 : 0;
}
