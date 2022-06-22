/*	$NetBSD: msg_281.c,v 1.4 2022/06/22 19:23:18 rillig Exp $	*/
# 3 "msg_281.c"

// Test for message: duplicate comment /* %s */ [281]

/* expect+1: warning: duplicate comment ** ARGSUSED ** [281] */
/* ARGSUSED *//* ARGSUSED */
void args_used(int x)
{
}
