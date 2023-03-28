/*	$NetBSD: msg_285.c,v 1.5 2023/03/28 14:44:35 rillig Exp $	*/
# 3 "msg_285.c"

// Test for message: prototype declaration [285]

/* lint1-extra-flags: -r -X 351 */

/* expect+1: prototype declaration [285] */
void function(int, int, int);

/* ARGSUSED */
extern void
function(a, b)
    int a, b;
/* expect+1: error: parameter mismatch: 3 declared, 2 defined [51] */
{
}
