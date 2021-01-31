/*	$NetBSD: msg_136.c,v 1.3 2021/01/31 11:12:07 rillig Exp $	*/
# 3 "msg_136.c"

// Test for message: cannot do pointer arithmetic on operand of unknown size [136]

struct incomplete;		/* expect: 233 */

const struct incomplete *
example(const struct incomplete *ptr)
{
	return ptr + 5;		/* expect: 136 */
}
