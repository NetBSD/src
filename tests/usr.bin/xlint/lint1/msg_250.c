/*	$NetBSD: msg_250.c,v 1.3 2021/06/29 07:17:43 rillig Exp $	*/
# 3 "msg_250.c"

// Test for message: unknown character \%o [250]

@deprecated			/* expect: unknown character \100 [250] */
char *gets(void);		/* expect: syntax error 'char' [249] */
