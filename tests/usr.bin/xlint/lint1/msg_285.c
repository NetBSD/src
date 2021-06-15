/*	$NetBSD: msg_285.c,v 1.3 2021/06/15 08:48:49 rillig Exp $	*/
# 3 "msg_285.c"

// Test for message: prototype declaration [285]

/* lint1-extra-flags: -r */

void function(int, int, int);	/* expect: 285 */

/* ARGSUSED */
extern void
function(a, b)
    int a, b;
{				/* expect: 3 declared, 2 defined */
}
