/*	$NetBSD: msg_002.c,v 1.3 2021/01/31 11:12:07 rillig Exp $	*/
# 3 "msg_002.c"

// Test for message: empty declaration [2]

int;				/* expect: 2 */

int local_variable;
