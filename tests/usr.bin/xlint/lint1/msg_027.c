/*	$NetBSD: msg_027.c,v 1.2 2021/01/03 15:35:00 rillig Exp $	*/
# 3 "msg_027.c"

// Test for message: redeclaration of %s [27]

extern int identifier(void);

extern double identifier(void);
