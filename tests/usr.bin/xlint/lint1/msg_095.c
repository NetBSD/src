/*	$NetBSD: msg_095.c,v 1.2 2021/01/07 00:38:46 rillig Exp $	*/
# 3 "msg_095.c"

// Test for message: declaration hides earlier one: %s [95]

/* lint1-flags: -ghSw */

int identifier;

int
example(int identifier)
{

	{
		int identifier = 3;
	}

	return identifier;
}
