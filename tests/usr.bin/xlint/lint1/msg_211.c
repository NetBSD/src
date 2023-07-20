/*	$NetBSD: msg_211.c,v 1.6 2023/07/07 19:45:22 rillig Exp $	*/
# 3 "msg_211.c"

// Test for message: function has return type '%s' but returns '%s' [211]

/* lint1-extra-flags: -X 351 */

struct str {
	int member;
};

int
return_int(double dbl, void *ptr, struct str str)
{
	if (dbl > 0.0)
		return dbl;
	if (ptr != (void *)0)
		/* expect+1: warning: illegal combination of integer 'int' and pointer 'pointer to void' [183] */
		return ptr;
	if (str.member > 0)
		/* expect+1: error: function has return type 'int' but returns 'struct str' [211] */
		return str;
	return 3;
}
