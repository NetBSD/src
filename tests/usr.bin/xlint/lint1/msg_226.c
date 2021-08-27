/*	$NetBSD: msg_226.c,v 1.3 2021/08/27 20:49:25 rillig Exp $	*/
# 3 "msg_226.c"

// Test for message: static variable %s unused [226]

/* expect+1: warning: static variable unused unused [226] */
static int unused;
