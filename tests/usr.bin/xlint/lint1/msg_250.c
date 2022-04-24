/*	$NetBSD: msg_250.c,v 1.4 2022/04/24 20:08:23 rillig Exp $	*/
# 3 "msg_250.c"

// Test for message: unknown character \%o [250]

/* expect+1: unknown character \100 [250] */
@deprecated
/* expect+2: error: old style declaration; add 'int' [1] */
/* expect+1: syntax error 'char' [249] */
char *gets(void);
