/*	$NetBSD: msg_068.c,v 1.2 2021/01/08 21:25:03 rillig Exp $	*/
# 3 "msg_068.c"

// Test for message: typedef already qualified with '%s' [68]

typedef const char const_char;

const const_char twice_const;
