/*	$NetBSD: msg_018.c,v 1.7 2025/04/12 15:49:50 rillig Exp $	*/
# 3 "msg_018.c"

// Test for message: invalid use of 'void' [18]

/* lint1-extra-flags: -X 351 */

/* expect+1: error: void type for 'x' [19] */
void x;

/* expect+1: error: cannot take size/alignment of void [146] */
unsigned long sizeof_void = sizeof(void);

/* expect+2: error: invalid use of 'void' [18] */
/* expect+1: warning: empty array declaration for 'void_array' [190] */
void void_array[];
