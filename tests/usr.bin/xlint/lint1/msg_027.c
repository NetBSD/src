/*	$NetBSD: msg_027.c,v 1.3 2021/01/31 11:12:07 rillig Exp $	*/
# 3 "msg_027.c"

// Test for message: redeclaration of %s [27]

extern int identifier(void);

extern double identifier(void);		/* expect: 27 */
