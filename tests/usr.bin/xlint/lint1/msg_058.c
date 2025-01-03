/*	$NetBSD: msg_058.c,v 1.7 2025/01/03 03:14:47 rillig Exp $	*/
# 3 "msg_058.c"

// Test for message: type of '%s' does not match prototype [58]

/* lint1-extra-flags: -X 351 */

int function(int, char, const char *);

int
/* expect+1: warning: function definition for 'function' with identifier list is obsolete in C23 [384] */
function(i, dbl, str)
	int i;
	double dbl;
	const char *str;
/* expect+1: error: type of 'dbl' does not match prototype [58] */
{
	return i + (int)dbl + str[0];
}
