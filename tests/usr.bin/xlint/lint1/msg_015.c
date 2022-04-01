/*	$NetBSD: msg_015.c,v 1.4 2022/04/01 22:07:23 rillig Exp $	*/
# 3 "msg_015.c"

// Test for message: function returns illegal type '%s' [15]

typedef int array[5];

/* expect+1: error: function returns illegal type 'array[5] of int' [15] */
array invalid(void);
