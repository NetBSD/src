/*	$NetBSD: msg_026.c,v 1.5 2023/03/28 14:44:34 rillig Exp $	*/
# 3 "msg_026.c"

// Test for message: cannot initialize extern declaration '%s' [26]

/* lint1-extra-flags: -X 351 */

/* expect+1: warning: cannot initialize extern declaration 'used' [26] */
extern int used = 1;
int defined = 1;
