/*	$NetBSD: msg_095.c,v 1.4 2022/04/03 09:34:45 rillig Exp $	*/
# 3 "msg_095.c"

// Test for message: declaration hides earlier one: %s [95]

/* lint1-flags: -ghSw */

int identifier;

int
example(int identifier)
{

	{
		/* expect+1: warning: declaration hides earlier one: identifier [95] */
		int identifier = 3;
	}

	return identifier;
}
