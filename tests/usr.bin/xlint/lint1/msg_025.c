/*	$NetBSD: msg_025.c,v 1.3 2021/08/27 20:16:50 rillig Exp $	*/
# 3 "msg_025.c"

// Test for message: cannot initialize typedef: %s [25]

/* expect+1: error: cannot initialize typedef: number [25] */
typedef int number = 3;
