/*	$NetBSD: msg_142.c,v 1.4 2021/04/05 01:35:34 rillig Exp $	*/
# 3 "msg_142.c"

// Test for message: floating point overflow detected, op %s [142]

double dbl = 1e100 * 1e100 * 1e100 * 1e100 * 1e100;	/* expect: 142 *//* expect: 142 */
