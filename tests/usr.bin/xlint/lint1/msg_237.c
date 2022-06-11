/*	$NetBSD: msg_237.c,v 1.3 2022/06/11 12:24:00 rillig Exp $	*/
# 3 "msg_237.c"

// Test for message: redeclaration of formal parameter '%s' [237]

/* See also message 21, which has the same text. */

/*ARGSUSED*/
void
/* expect+1: error: redeclaration of formal parameter 'param' [237] */
prototype_with_duplicate_parameter(int param, int param)
{
}
