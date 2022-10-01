/*	$NetBSD: msg_250.c,v 1.6 2022/10/01 09:42:40 rillig Exp $	*/
# 3 "msg_250.c"

// Test for message: unknown character \%o [250]

/* expect+1: error: unknown character \100 [250] */
@deprecated
/* expect+2: error: old-style declaration; add 'int' [1] */
/* expect+1: error: syntax error 'char' [249] */
char *gets(void);
