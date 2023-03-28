/*	$NetBSD: msg_068.c,v 1.5 2023/03/28 14:44:34 rillig Exp $	*/
# 3 "msg_068.c"

// Test for message: typedef already qualified with '%s' [68]

/* lint1-extra-flags: -X 351 */

typedef const char const_char;

/* expect+1: warning: typedef already qualified with 'const' [68] */
const const_char twice_const;
