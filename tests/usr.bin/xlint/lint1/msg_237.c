/*	$NetBSD: msg_237.c,v 1.4 2023/07/07 19:45:22 rillig Exp $	*/
# 3 "msg_237.c"

// Test for message: redeclaration of formal parameter '%s' [237]

/* See also message 21, which has the same text. */

/* lint1-extra-flags: -X 351 */

/*ARGSUSED*/
void
/* expect+1: error: redeclaration of formal parameter 'param' [237] */
prototype_with_duplicate_parameter(int param, int param)
{
}
