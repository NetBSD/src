/*	$NetBSD: msg_157.c,v 1.3 2021/03/16 23:39:41 rillig Exp $	*/
# 3 "msg_157.c"

// Test for message: ANSI C treats constant as unsigned [157]

/*
 * A rather strange definition for an ARGB color.
 * Luckily, 'double' has more than 32 significant binary digits.
 */
double white = 0xFFFFFFFF;	/* expect: 157 */
