/*	$NetBSD: msg_227.c,v 1.4 2022/06/22 19:23:18 rillig Exp $	*/
# 3 "msg_227.c"

// Test for message: const object '%s' should have initializer [227]

/* expect+2: warning: static variable 'without_initializer' unused [226] */
/* expect+1: warning: const object 'without_initializer' should have initializer [227] */
static const int without_initializer;
/* expect+1: warning: static variable 'with_initializer' unused [226] */
static const int with_initializer = 1;
