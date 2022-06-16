/*	$NetBSD: msg_181.c,v 1.4 2022/06/16 16:58:36 rillig Exp $	*/
# 3 "msg_181.c"

// Test for message: {}-enclosed initializer required [181]

/* expect+1: error: {}-enclosed initializer required [181] */
struct { int x; } missing_braces = 3;
struct { int x; } including_braces = { 3 };
