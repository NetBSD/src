/*	$NetBSD: msg_190.c,v 1.5 2023/03/28 14:44:35 rillig Exp $	*/
# 3 "msg_190.c"

// Test for message: empty array declaration for '%s' [190]

/* lint1-extra-flags: -X 351 */

/* expect+1: error: empty array declaration for 'empty_array' [190] */
double empty_array[] = {};

double array[] = { 1 };
