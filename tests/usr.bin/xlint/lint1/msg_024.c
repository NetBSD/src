/*	$NetBSD: msg_024.c,v 1.3 2021/08/27 20:16:50 rillig Exp $	*/
# 3 "msg_024.c"

// Test for message: cannot initialize function: %s [24]

typedef void (function)(void);

void
definition(void)
{
}

/* expect+3: error: cannot initialize function: fn [24] */
/* The following message is strange but does not occur in practice. */
/* expect+1: error: {}-enclosed initializer required [181] */
function fn = definition;
