/*	$NetBSD: msg_187.c,v 1.6 2024/03/27 19:28:20 rillig Exp $	*/
# 3 "msg_187.c"

// Test for message: string literal too long (%ju) for target array (%ju) [187]

/* lint1-extra-flags: -X 351 */

char auto_msg[] = "the string length is determined automatically";

/* The terminating null byte is not copied to the array. */
char large_enough[10] = "0123456789";

/* expect+1: warning: string literal too long (10) for target array (9) [187] */
char too_small[9] = "0123456789";

char x0[3] = "x\0";

char xx0[3] = "xx\0";

/* expect+1: warning: string literal too long (4) for target array (3) [187] */
char xxx0[3] = "012\0";

/* expect+1: warning: string literal too long (4) for target array (3) [187] */
char xx00[3] = "01\0\0";
