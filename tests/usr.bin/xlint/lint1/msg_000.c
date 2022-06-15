/*	$NetBSD: msg_000.c,v 1.4 2022/06/15 20:18:31 rillig Exp $	*/
# 3 "msg_000.c"

// Test for message: empty declaration [0]

extern int extern_declared;

/* expect+1: warning: empty declaration [0] */
;

/* expect+1: warning: static variable 'local_defined' unused [226] */
static int local_defined;
