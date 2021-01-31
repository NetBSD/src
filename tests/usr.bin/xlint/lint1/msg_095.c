/*	$NetBSD: msg_095.c,v 1.3 2021/01/31 11:12:07 rillig Exp $	*/
# 3 "msg_095.c"

// Test for message: declaration hides earlier one: %s [95]

/* lint1-flags: -ghSw */

int identifier;

int
example(int identifier)
{

	{
		int identifier = 3;	/* expect: 95 */
	}

	return identifier;
}
