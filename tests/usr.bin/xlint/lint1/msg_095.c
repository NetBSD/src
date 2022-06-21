/*	$NetBSD: msg_095.c,v 1.5 2022/06/21 21:18:30 rillig Exp $	*/
# 3 "msg_095.c"

// Test for message: declaration of '%s' hides earlier one [95]

/* lint1-flags: -ghSw */

int identifier;

int
example(int identifier)
{

	{
		/* expect+1: warning: declaration of 'identifier' hides earlier one [95] */
		int identifier = 3;
	}

	return identifier;
}
