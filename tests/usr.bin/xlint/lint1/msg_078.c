/*	$NetBSD: msg_078.c,v 1.5 2023/03/28 14:44:34 rillig Exp $	*/
# 3 "msg_078.c"

// Test for message: nonportable character escape [78]
/* This message is not used. */

/* lint1-extra-flags: -X 351 */

char either_255_or_minus_1 = '\377';
/* expect+1: warning: dubious escape \y [79] */
char dubious_escape = '\y';
