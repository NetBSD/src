/*	$NetBSD: msg_190.c,v 1.3 2021/07/10 09:24:27 rillig Exp $	*/
# 3 "msg_190.c"

// Test for message: empty array declaration: %s [190]

/* expect+1: error: empty array declaration: empty_array [190] */
double empty_array[] = {};

double array[] = { 1 };
