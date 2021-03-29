/*	$NetBSD: msg_181.c,v 1.3 2021/03/29 22:59:03 rillig Exp $	*/
# 3 "msg_181.c"

// Test for message: {}-enclosed initializer required [181]

struct { int x; } missing_braces = 3;		/* expect: 181 */
struct { int x; } including_braces = { 3 };
