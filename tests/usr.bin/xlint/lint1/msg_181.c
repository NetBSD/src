/*	$NetBSD: msg_181.c,v 1.5 2023/03/28 14:44:35 rillig Exp $	*/
# 3 "msg_181.c"

// Test for message: {}-enclosed initializer required [181]

/* lint1-extra-flags: -X 351 */

/* expect+1: error: {}-enclosed initializer required [181] */
struct { int x; } missing_braces = 3;
struct { int x; } including_braces = { 3 };
