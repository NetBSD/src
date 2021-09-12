/*	$NetBSD: msg_027.c,v 1.5 2021/09/12 16:28:45 rillig Exp $	*/
# 3 "msg_027.c"

// Test for message: redeclaration of %s [27]

extern int identifier(void);

/* expect+1: error: redeclaration of 'identifier' with type 'function(void) returning double', expected 'function(void) returning int' [347] */
extern double identifier(void);
