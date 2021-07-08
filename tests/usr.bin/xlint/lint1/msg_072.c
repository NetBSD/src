/*	$NetBSD: msg_072.c,v 1.4 2021/07/08 05:18:49 rillig Exp $	*/
# 3 "msg_072.c"

// Test for message: typedef declares no type name [72]

/* expect+1: warning: typedef declares no type name [72] */
typedef int;

typedef int number;

/* expect+1: warning: typedef declares no type name [72] */
const typedef;
