/*	$NetBSD: msg_285.c,v 1.7 2025/01/03 03:14:47 rillig Exp $	*/
# 3 "msg_285.c"

// Test for message: prototype declaration [285]

/* lint1-extra-flags: -r -X 351 */

/* expect+1: prototype declaration [285] */
void function(int, int, int);

/* ARGSUSED */
extern void
/* expect+1: warning: function definition for 'function' with identifier list is obsolete in C23 [384] */
function(a, b)
    int a, b;
/* expect+1: error: parameter mismatch: 3 declared, 2 defined [51] */
{
}
