/*	$NetBSD: msg_073.c,v 1.4 2022/06/15 20:18:31 rillig Exp $	*/
# 3 "msg_073.c"

// Test for message: empty character constant [73]

/* expect+1: error: empty character constant [73] */
char empty = '';
char letter = 'x';
