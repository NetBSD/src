/*	$NetBSD: msg_058.c,v 1.5.2.1 2025/08/02 05:58:15 perseant Exp $	*/
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
