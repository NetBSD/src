/*	$NetBSD: msg_086.c,v 1.4 2022/06/15 20:18:31 rillig Exp $	*/
# 3 "msg_086.c"

// Test for message: automatic hides external declaration: %s [86]

/* lint1-flags: -S -g -h -w */

extern int identifier;

int
local_auto(void)
{
	/* expect+1: warning: automatic hides external declaration: identifier [86] */
	int identifier = 3;
	return identifier;
}

/* XXX: the function argument does not trigger the warning. */
int
arg_auto(int identifier)
{
	return identifier;
}
