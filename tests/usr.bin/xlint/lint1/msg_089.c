/*	$NetBSD: msg_089.c,v 1.5 2022/06/19 12:14:34 rillig Exp $	*/
# 3 "msg_089.c"

// Test for message: typedef '%s' redeclared [89]

typedef int number;
/* expect+1: error: typedef 'number' redeclared [89] */
typedef int number;
/* expect+1: error: typedef 'number' redeclared [89] */
typedef double number;
