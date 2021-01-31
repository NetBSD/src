/*	$NetBSD: msg_086.c,v 1.3 2021/01/31 11:12:07 rillig Exp $	*/
# 3 "msg_086.c"

// Test for message: automatic hides external declaration: %s [86]

/* lint1-flags: -S -g -h -w */

extern int identifier;

int
local_auto(void)
{
	int identifier = 3;	/* expect: 86 */
	return identifier;
}

/* XXX: the function argument does not trigger the warning. */
int
arg_auto(int identifier)
{
	return identifier;
}
