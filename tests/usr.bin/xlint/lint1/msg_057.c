/*	$NetBSD: msg_057.c,v 1.3 2021/01/31 11:12:07 rillig Exp $	*/
# 3 "msg_057.c"

// Test for message: enumeration constant hides parameter: %s [57]

long
rgb(int red, int green, int blue)	/* expect: 231, 231, 231 */
{
	enum color {
		red, green, blue	/* expect: 57, 57 */
	};				/* expect: 57 */

	return red + green + blue;
}
