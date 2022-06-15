/*	$NetBSD: msg_006.c,v 1.5 2022/06/15 20:18:31 rillig Exp $	*/
# 3 "msg_006.c"

// Test for message: use 'double' instead of 'long float' [6]

/* expect+2: warning: use 'double' instead of 'long float' [6] */
/* expect+1: error: illegal type combination [4] */
long float x;
double x;
