/*	$NetBSD: msg_157.c,v 1.6 2023/08/26 10:43:53 rillig Exp $	*/
# 3 "msg_157.c"

/* Test for message: C90 treats constant as unsigned [157] */

/* lint1-flags: -w -X 351 */

/*
 * A rather strange definition for an ARGB color.
 * Luckily, 'double' has more than 32 significant binary digits.
 */
/* expect+1: warning: C90 treats constant as unsigned [157] */
double white = 0xFFFFFFFF;
