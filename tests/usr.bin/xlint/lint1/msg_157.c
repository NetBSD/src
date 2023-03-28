/*	$NetBSD: msg_157.c,v 1.5 2023/03/28 14:44:35 rillig Exp $	*/
# 3 "msg_157.c"

/* Test for message: ANSI C treats constant as unsigned [157] */

/* lint1-flags: -w -X 351 */

/*
 * A rather strange definition for an ARGB color.
 * Luckily, 'double' has more than 32 significant binary digits.
 */
/* expect+1: warning: ANSI C treats constant as unsigned [157] */
double white = 0xFFFFFFFF;
