/*	$NetBSD: msg_289.c,v 1.3 2021/08/27 20:49:25 rillig Exp $	*/
# 3 "msg_289.c"

// Test for message: can't be used together: /* PRINTFLIKE */ /* SCANFLIKE */ [289]

/* PRINTFLIKE */ /* SCANFLIKE */
void
both(void)
/* expect+1: warning: can't be used together */
{
}
