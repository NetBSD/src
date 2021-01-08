/*	$NetBSD: msg_164.c,v 1.2 2021/01/08 21:25:03 rillig Exp $	*/
# 3 "msg_164.c"

// Test for message: assignment of negative constant to unsigned type [164]

void
example(void)
{
	unsigned char uch = -3;

	uch = -5;
	uch += -7;
	uch *= -1;
}
