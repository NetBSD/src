/*	$NetBSD: msg_015.c,v 1.5 2023/03/28 14:44:34 rillig Exp $	*/
# 3 "msg_015.c"

// Test for message: function returns illegal type '%s' [15]

/* lint1-extra-flags: -X 351 */

typedef int array[5];

/* expect+1: error: function returns illegal type 'array[5] of int' [15] */
array invalid(void);
