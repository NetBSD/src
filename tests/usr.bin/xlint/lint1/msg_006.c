/*	$NetBSD: msg_006.c,v 1.6 2023/03/28 14:44:34 rillig Exp $	*/
# 3 "msg_006.c"

// Test for message: use 'double' instead of 'long float' [6]

/* lint1-extra-flags: -X 351 */

/* expect+2: warning: use 'double' instead of 'long float' [6] */
/* expect+1: error: illegal type combination [4] */
long float x;
double x;
