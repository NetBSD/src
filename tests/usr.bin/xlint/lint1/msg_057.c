/*	$NetBSD: msg_057.c,v 1.6 2022/06/20 21:13:36 rillig Exp $	*/
# 3 "msg_057.c"

// Test for message: enumeration constant '%s' hides parameter [57]

long
/* expect+3: warning: argument 'red' unused in function 'rgb' [231] */
/* expect+2: warning: argument 'green' unused in function 'rgb' [231] */
/* expect+1: warning: argument 'blue' unused in function 'rgb' [231] */
rgb(int red, int green, int blue)
{
	enum color {
		/* expect+2: warning: enumeration constant 'red' hides parameter [57] */
		/* expect+1: warning: enumeration constant 'green' hides parameter [57] */
		red, green, blue
	};
	/* expect-1: warning: enumeration constant 'blue' hides parameter [57] */
	/*
	 * The warning for 'blue' is at the semicolon since the parser has
	 * already advanced that far, checking for an optional initializer.
	 * As of 2022-06-15, lint does not keep track of the location of each
	 * individual token.
	 */

	return red + green + blue;
}
