/*	$NetBSD: msg_282.c,v 1.4 2022/06/22 19:23:18 rillig Exp $	*/
# 3 "msg_282.c"

// Test for message: comment /* %s */ must precede function definition [282]

/* expect+1: warning: comment ** ARGSUSED ** must precede function definition [282] */
/* ARGSUSED */
int argsused;

/* expect+1: warning: comment ** VARARGS ** must precede function definition [282] */
/* VARARGS */
int varargs;

/* expect+1: warning: comment ** PRINTFLIKE ** must precede function definition [282] */
/* PRINTFLIKE */
int printflike;

/* expect+1: warning: comment ** SCANFLIKE ** must precede function definition [282] */
/* SCANFLIKE */
int scanflike;
