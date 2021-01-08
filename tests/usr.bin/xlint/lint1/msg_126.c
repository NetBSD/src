/*	$NetBSD: msg_126.c,v 1.2 2021/01/08 21:25:03 rillig Exp $	*/
# 3 "msg_126.c"

// Test for message: incompatible types in conditional [126]

int
max(int cond, void *ptr, double dbl)
{
	return cond ? ptr : dbl;
}
