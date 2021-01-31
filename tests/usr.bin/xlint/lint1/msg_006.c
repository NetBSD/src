/*	$NetBSD: msg_006.c,v 1.3 2021/01/31 11:12:07 rillig Exp $	*/
# 3 "msg_006.c"

// Test for message: use 'double' instead of 'long float' [6]

long float x;			/* expect: 6, 4 */
double x;
