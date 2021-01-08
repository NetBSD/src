/*	$NetBSD: msg_142.c,v 1.2 2021/01/08 21:25:03 rillig Exp $	*/
# 3 "msg_142.c"

// Test for message: floating point overflow detected, op %s [142]

double dbl = 1e100 * 1e100 * 1e100 * 1e100 * 1e100;
