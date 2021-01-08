/*	$NetBSD: msg_057.c,v 1.2 2021/01/08 21:25:03 rillig Exp $	*/
# 3 "msg_057.c"

// Test for message: enumeration constant hides parameter: %s [57]

long
rgb(int red, int green, int blue)
{
	enum color {
		red, green, blue
	};

	return red + green + blue;
}
