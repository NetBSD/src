/*	$NetBSD: msg_072.c,v 1.3 2021/01/31 11:12:07 rillig Exp $	*/
# 3 "msg_072.c"

// Test for message: typedef declares no type name [72]

typedef int;			/* expect: 72 */

typedef int number;
