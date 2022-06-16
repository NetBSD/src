/*	$NetBSD: msg_136.c,v 1.4 2022/06/16 16:58:36 rillig Exp $	*/
# 3 "msg_136.c"

// Test for message: cannot do pointer arithmetic on operand of unknown size [136]

/* expect+1: warning: struct 'incomplete' never defined [233] */
struct incomplete;

const struct incomplete *
example(const struct incomplete *ptr)
{
	/* expect+1: error: cannot do pointer arithmetic on operand of unknown size [136] */
	return ptr + 5;
}
