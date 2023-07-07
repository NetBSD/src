/*	$NetBSD: msg_281.c,v 1.5 2023/07/07 19:45:22 rillig Exp $	*/
# 3 "msg_281.c"

// Test for message: duplicate comment /* %s */ [281]

/* lint1-extra-flags: -X 351 */

/* expect+1: warning: duplicate comment ** ARGSUSED ** [281] */
/* ARGSUSED *//* ARGSUSED */
void args_used(int x)
{
}
