/*	$NetBSD: msg_057.c,v 1.4 2021/04/05 01:35:34 rillig Exp $	*/
# 3 "msg_057.c"

// Test for message: enumeration constant hides parameter: %s [57]

long
rgb(int red, int green, int blue)	/* expect: 231 *//* expect: 231 *//* expect: 231 */
{
	enum color {
		red, green, blue	/* expect: 57 *//* expect: 57 */
	};				/* expect: 57 */

	return red + green + blue;
}
