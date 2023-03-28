/*	$NetBSD: msg_282.c,v 1.5 2023/03/28 14:44:35 rillig Exp $	*/
# 3 "msg_282.c"

// Test for message: comment /* %s */ must precede function definition [282]

/* lint1-extra-flags: -X 351 */

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
