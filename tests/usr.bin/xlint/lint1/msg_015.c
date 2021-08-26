/*	$NetBSD: msg_015.c,v 1.3 2021/08/26 19:23:25 rillig Exp $	*/
# 3 "msg_015.c"

// Test for message: function returns illegal type [15]

typedef int array[5];

/* expect+1: error: function returns illegal type [15] */
array invalid(void);
