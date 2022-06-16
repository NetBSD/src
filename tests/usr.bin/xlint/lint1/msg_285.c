/*	$NetBSD: msg_285.c,v 1.4 2022/06/16 21:24:41 rillig Exp $	*/
# 3 "msg_285.c"

// Test for message: prototype declaration [285]

/* lint1-extra-flags: -r */

/* expect+1: prototype declaration [285] */
void function(int, int, int);

/* ARGSUSED */
extern void
function(a, b)
    int a, b;
/* expect+1: error: parameter mismatch: 3 declared, 2 defined [51] */
{
}
