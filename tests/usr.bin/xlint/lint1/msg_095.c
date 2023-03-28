/*	$NetBSD: msg_095.c,v 1.6 2023/03/28 14:44:34 rillig Exp $	*/
# 3 "msg_095.c"

// Test for message: declaration of '%s' hides earlier one [95]

/* lint1-flags: -ghSw -X 351 */

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
