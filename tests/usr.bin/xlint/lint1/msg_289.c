/*	$NetBSD: msg_289.c,v 1.5 2023/07/07 19:45:22 rillig Exp $	*/
# 3 "msg_289.c"

// Test for message: can't be used together: /* PRINTFLIKE */ /* SCANFLIKE */ [289]

/* lint1-extra-flags: -X 351 */

/* PRINTFLIKE */ /* SCANFLIKE */
void
both(void)
/* expect+1: warning: can't be used together: ** PRINTFLIKE ** ** SCANFLIKE ** [289] */
{
}
