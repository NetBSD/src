/*	$NetBSD: msg_095.c,v 1.7 2023/07/07 06:03:31 rillig Exp $	*/
# 3 "msg_095.c"

// Test for message: declaration of '%s' hides earlier one [95]

/* lint1-flags: -ghSw -X 351 */

int identifier;

int
example(int identifier)
{

	{
		/* expect+2: warning: 'identifier' set but not used in function 'example' [191] */
		/* expect+1: warning: declaration of 'identifier' hides earlier one [95] */
		int identifier = 3;
	}

	return identifier;
}
