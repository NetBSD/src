/*	$NetBSD: msg_078.c,v 1.4 2021/08/27 20:19:45 rillig Exp $	*/
# 3 "msg_078.c"

// Test for message: nonportable character escape [78]
/* This message is not used. */

char either_255_or_minus_1 = '\377';
/* expect+1: warning: dubious escape \y [79] */
char dubious_escape = '\y';
