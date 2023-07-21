/*	$NetBSD: msg_024.c,v 1.6 2023/07/21 06:02:07 rillig Exp $	*/
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
/* expect+1: error: {}-enclosed or constant initializer of type 'function(void) returning void' required [181] */
function fn = definition;
