/*	$NetBSD: msg_086.c,v 1.2 2021/01/03 15:35:00 rillig Exp $	*/
# 3 "msg_086.c"

// Test for message: automatic hides external declaration: %s [86]

/* lint1-flags: -S -g -h -w */

extern int identifier;

int
local_auto(void)
{
	int identifier = 3;
	return identifier;
}

/* XXX: the function argument does not trigger the warning. */
int
arg_auto(int identifier)
{
	return identifier;
}
