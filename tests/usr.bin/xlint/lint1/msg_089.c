/*	$NetBSD: msg_089.c,v 1.2 2021/01/08 21:25:03 rillig Exp $	*/
# 3 "msg_089.c"

// Test for message: typedef redeclared: %s [89]

typedef int number;
typedef int number;
typedef double number;
