/*	$NetBSD: msg_058.c,v 1.3 2022/02/27 20:02:44 rillig Exp $	*/
# 3 "msg_058.c"

// Test for message: type does not match prototype: %s [58]

int function(int, char, const char *);

int
function(i, dbl, str)
	int i;
	double dbl;
	const char *str;
/* expect+1: error: type does not match prototype: dbl [58] */
{
	return i + (int)dbl + str[0];
}
