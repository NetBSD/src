/*	$NetBSD: msg_073.c,v 1.5 2023/03/28 14:44:34 rillig Exp $	*/
# 3 "msg_073.c"

// Test for message: empty character constant [73]

/* lint1-extra-flags: -X 351 */

/* expect+1: error: empty character constant [73] */
char empty = '';
char letter = 'x';
