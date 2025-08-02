/*	$NetBSD: msg_015.c,v 1.5.2.1 2025/08/02 05:58:15 perseant Exp $	*/
# 3 "msg_015.c"

// Test for message: function returns invalid type '%s' [15]

/* lint1-extra-flags: -X 351 */

typedef int array[5];

/* expect+1: error: function returns invalid type 'array[5] of int' [15] */
array invalid(void);
