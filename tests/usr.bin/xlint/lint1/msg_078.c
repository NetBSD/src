/*	$NetBSD: msg_078.c,v 1.3 2021/08/27 20:16:50 rillig Exp $	*/
# 3 "msg_078.c"

// Test for message: nonportable character escape [78]

char either_255_or_minus_1 = '\377';
/* expect+1: warning: dubious escape \y [79] */
char dubious_escape = '\y';
