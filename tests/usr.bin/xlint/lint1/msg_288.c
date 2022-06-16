/*	$NetBSD: msg_288.c,v 1.3 2022/06/16 21:24:41 rillig Exp $	*/
# 3 "msg_288.c"

// Test for message: dubious use of /* VARARGS */ with /* %s */ [288]

/* VARARGS */
int
just_varargs(char x)
{
	/*
	 * No warning here, even though having a VARARGS annotation on a
	 * function that is incompatible to varargs is dubious.
	 */
	return x;
}

/* VARARGS */
/* PRINTFLIKE */
int
example(int x)
/* expect+1: warning: dubious use of ** VARARGS ** with ** PRINTFLIKE ** [288] */
{
	return x;
}
