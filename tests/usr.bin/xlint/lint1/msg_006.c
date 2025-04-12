/*	$NetBSD: msg_006.c,v 1.7 2025/04/12 15:49:50 rillig Exp $	*/
# 3 "msg_006.c"

// Test for message: use 'double' instead of 'long float' [6]

/* lint1-extra-flags: -X 351 */

/* expect+2: warning: use 'double' instead of 'long float' [6] */
/* expect+1: error: invalid type combination [4] */
long float x;
double x;
