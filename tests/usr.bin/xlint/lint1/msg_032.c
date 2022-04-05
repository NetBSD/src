/*	$NetBSD: msg_032.c,v 1.3 2022/04/05 23:09:19 rillig Exp $	*/
# 3 "msg_032.c"

// Test for message: argument type defaults to 'int': %s [32]

add(a, b, c)
/* expect+3: warning: argument type defaults to 'int': a [32] */
/* expect+2: warning: argument type defaults to 'int': b [32] */
/* expect+1: warning: argument type defaults to 'int': c [32] */
{
	return a + b + c;
}
