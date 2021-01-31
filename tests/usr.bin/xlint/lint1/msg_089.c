/*	$NetBSD: msg_089.c,v 1.3 2021/01/31 11:12:07 rillig Exp $	*/
# 3 "msg_089.c"

// Test for message: typedef redeclared: %s [89]

typedef int number;
typedef int number;		/* expect: 89 */
typedef double number;		/* expect: 89 */
