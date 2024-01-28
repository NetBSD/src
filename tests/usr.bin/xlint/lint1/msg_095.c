/*	$NetBSD: msg_095.c,v 1.8 2024/01/28 08:17:27 rillig Exp $	*/
# 3 "msg_095.c"

// Test for message: declaration of '%s' hides earlier one [95]

/* lint1-flags: -ghSw -X 351 */

int identifier;

int
example(int identifier)
{

	{
		/* expect+2: warning: declaration of 'identifier' hides earlier one [95] */
		/* expect+1: warning: 'identifier' set but not used in function 'example' [191] */
		int identifier = 3;
	}

	return identifier;
}
