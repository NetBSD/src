/*	$NetBSD: msg_142.c,v 1.3 2021/01/31 11:12:07 rillig Exp $	*/
# 3 "msg_142.c"

// Test for message: floating point overflow detected, op %s [142]

double dbl = 1e100 * 1e100 * 1e100 * 1e100 * 1e100;	/* expect: 142, 142 */
