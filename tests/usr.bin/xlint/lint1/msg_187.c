/*	$NetBSD: msg_187.c,v 1.3 2021/09/02 18:20:00 rillig Exp $	*/
# 3 "msg_187.c"

// Test for message: non-null byte ignored in string initializer [187]

char auto_msg[] = "the string length is determined automatically";

char large_enough[10] = "0123456789";

/* expect+1: warning: non-null byte ignored in string initializer [187] */
char too_small[9] = "0123456789";

char x0[3] = "x\0";

char xx0[3] = "xx\0";

/* expect+1: warning: non-null byte ignored in string initializer [187] */
char xxx0[3] = "012\0";

/*
 * The warning is not entirely correct.  It is a non-terminating byte that
 * is ignored.
 */
/* expect+1: warning: non-null byte ignored in string initializer [187] */
char xx00[3] = "01\0\0";
