/*	$NetBSD: msg_024.c,v 1.4 2022/06/19 11:50:42 rillig Exp $	*/
# 3 "msg_024.c"

// Test for message: cannot initialize function '%s' [24]

typedef void (function)(void);

void
definition(void)
{
}

/* expect+3: error: cannot initialize function 'fn' [24] */
/* The following message is strange but does not occur in practice. */
/* expect+1: error: {}-enclosed initializer required [181] */
function fn = definition;
