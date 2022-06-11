/*	$NetBSD: msg_226.c,v 1.4 2022/06/11 11:52:13 rillig Exp $	*/
# 3 "msg_226.c"

// Test for message: static variable '%s' unused [226]

/* expect+1: warning: static variable 'unused' unused [226] */
static int unused;
