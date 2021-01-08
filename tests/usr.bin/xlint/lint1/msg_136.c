/*	$NetBSD: msg_136.c,v 1.2 2021/01/08 21:25:03 rillig Exp $	*/
# 3 "msg_136.c"

// Test for message: cannot do pointer arithmetic on operand of unknown size [136]

struct incomplete;

const struct incomplete *
example(const struct incomplete *ptr)
{
	return ptr + 5;
}
