/*	$NetBSD: msg_015.c,v 1.6 2025/04/12 15:49:50 rillig Exp $	*/
# 3 "msg_015.c"

// Test for message: function returns invalid type '%s' [15]

/* lint1-extra-flags: -X 351 */

typedef int array[5];

/* expect+1: error: function returns invalid type 'array[5] of int' [15] */
array invalid(void);
