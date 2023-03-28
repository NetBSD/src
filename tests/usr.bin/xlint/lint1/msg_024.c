/*	$NetBSD: msg_024.c,v 1.5 2023/03/28 14:44:34 rillig Exp $	*/
# 3 "msg_024.c"

// Test for message: cannot initialize function '%s' [24]

/* lint1-extra-flags: -X 351 */

typedef void (function)(void);

void
definition(void)
{
}

/* expect+3: error: cannot initialize function 'fn' [24] */
/* The following message is strange but does not occur in practice. */
/* expect+1: error: {}-enclosed initializer required [181] */
function fn = definition;
