/*	$NetBSD: msg_058.c,v 1.5 2023/03/28 14:44:34 rillig Exp $	*/
# 3 "msg_058.c"

// Test for message: type of '%s' does not match prototype [58]

/* lint1-extra-flags: -X 351 */

int function(int, char, const char *);

int
function(i, dbl, str)
	int i;
	double dbl;
	const char *str;
/* expect+1: error: type of 'dbl' does not match prototype [58] */
{
	return i + (int)dbl + str[0];
}
