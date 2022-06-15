/*	$NetBSD: msg_068.c,v 1.4 2022/06/15 20:18:31 rillig Exp $	*/
# 3 "msg_068.c"

// Test for message: typedef already qualified with '%s' [68]

typedef const char const_char;

/* expect+1: warning: typedef already qualified with 'const' [68] */
const const_char twice_const;
