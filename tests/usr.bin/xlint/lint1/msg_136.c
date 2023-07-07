/*	$NetBSD: msg_136.c,v 1.5 2023/07/07 19:45:22 rillig Exp $	*/
# 3 "msg_136.c"

// Test for message: cannot do pointer arithmetic on operand of unknown size [136]

/* lint1-extra-flags: -X 351 */

/* expect+1: warning: struct 'incomplete' never defined [233] */
struct incomplete;

const struct incomplete *
example(const struct incomplete *ptr)
{
	/* expect+1: error: cannot do pointer arithmetic on operand of unknown size [136] */
	return ptr + 5;
}
