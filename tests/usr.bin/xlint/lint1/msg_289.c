/*	$NetBSD: msg_289.c,v 1.6 2023/07/09 11:01:27 rillig Exp $	*/
# 3 "msg_289.c"

// Test for message: /* PRINTFLIKE */ and /* SCANFLIKE */ cannot be combined [289]

/* lint1-extra-flags: -X 351 */

/* PRINTFLIKE */ /* SCANFLIKE */
void
both(void)
/* expect+1: warning: ** PRINTFLIKE ** and ** SCANFLIKE ** cannot be combined [289] */
{
}
