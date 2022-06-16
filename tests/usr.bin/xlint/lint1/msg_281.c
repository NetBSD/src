/*	$NetBSD: msg_281.c,v 1.3 2022/06/16 21:24:41 rillig Exp $	*/
# 3 "msg_281.c"

// Test for message: duplicate use of /* %s */ [281]

/* expect+1: warning: duplicate use of ** ARGSUSED ** [281] */
/* ARGSUSED *//* ARGSUSED */
void args_used(int x)
{
}
