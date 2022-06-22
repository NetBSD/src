/*	$NetBSD: msg_190.c,v 1.4 2022/06/22 19:23:18 rillig Exp $	*/
# 3 "msg_190.c"

// Test for message: empty array declaration for '%s' [190]

/* expect+1: error: empty array declaration for 'empty_array' [190] */
double empty_array[] = {};

double array[] = { 1 };
