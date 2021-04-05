/*	$NetBSD: msg_006.c,v 1.4 2021/04/05 01:35:34 rillig Exp $	*/
# 3 "msg_006.c"

// Test for message: use 'double' instead of 'long float' [6]

long float x;			/* expect: 6 *//* expect: 4 */
double x;
