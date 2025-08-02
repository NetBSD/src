/*	$NetBSD: msg_006.c,v 1.6.2.1 2025/08/02 05:58:15 perseant Exp $	*/
# 3 "msg_006.c"

// Test for message: use 'double' instead of 'long float' [6]

/* lint1-extra-flags: -X 351 */

/* expect+2: warning: use 'double' instead of 'long float' [6] */
/* expect+1: error: invalid type combination [4] */
long float x;
double x;
