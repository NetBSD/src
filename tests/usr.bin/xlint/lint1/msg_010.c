/*	$NetBSD: msg_010.c,v 1.2 2021/01/07 18:37:41 rillig Exp $	*/
# 3 "msg_010.c"

// Test for message: duplicate '%s' [10]

inline inline void
double_inline(void)
{
}

const const int
double_const(void)
{
	return 0;
}

volatile volatile int
double_volatile(void)
{
	return 0;
}
