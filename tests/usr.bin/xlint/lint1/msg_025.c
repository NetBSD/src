/*	$NetBSD: msg_025.c,v 1.4 2022/06/19 11:50:42 rillig Exp $	*/
# 3 "msg_025.c"

// Test for message: cannot initialize typedef '%s' [25]

/* expect+1: error: cannot initialize typedef 'number' [25] */
typedef int number = 3;
