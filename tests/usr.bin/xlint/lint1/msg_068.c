/*	$NetBSD: msg_068.c,v 1.3 2021/01/31 11:12:07 rillig Exp $	*/
# 3 "msg_068.c"

// Test for message: typedef already qualified with '%s' [68]

typedef const char const_char;

const const_char twice_const;	/* expect: 68 */
