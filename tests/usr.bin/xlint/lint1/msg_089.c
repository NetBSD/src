/*	$NetBSD: msg_089.c,v 1.4 2022/06/15 20:18:31 rillig Exp $	*/
# 3 "msg_089.c"

// Test for message: typedef redeclared: %s [89]

typedef int number;
/* expect+1: error: typedef redeclared: number [89] */
typedef int number;
/* expect+1: error: typedef redeclared: number [89] */
typedef double number;
