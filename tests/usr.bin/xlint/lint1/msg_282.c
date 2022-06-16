/*	$NetBSD: msg_282.c,v 1.3 2022/06/16 21:24:41 rillig Exp $	*/
# 3 "msg_282.c"

// Test for message: must precede function definition: /* %s */ [282]

/* expect+1: warning: must precede function definition: ** ARGSUSED ** [282] */
/* ARGSUSED */
int argsused;

/* expect+1: warning: must precede function definition: ** VARARGS ** [282] */
/* VARARGS */
int varargs;

/* expect+1: warning: must precede function definition: ** PRINTFLIKE ** [282] */
/* PRINTFLIKE */
int printflike;

/* expect+1: warning: must precede function definition: ** SCANFLIKE ** [282] */
/* SCANFLIKE */
int scanflike;
