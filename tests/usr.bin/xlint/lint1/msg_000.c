/*	$NetBSD: msg_000.c,v 1.5 2023/03/28 14:44:34 rillig Exp $	*/
# 3 "msg_000.c"

// Test for message: empty declaration [0]

/* lint1-extra-flags: -X 351 */

extern int extern_declared;

/* expect+1: warning: empty declaration [0] */
;

/* expect+1: warning: static variable 'local_defined' unused [226] */
static int local_defined;
