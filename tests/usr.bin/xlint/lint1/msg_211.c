/*	$NetBSD: msg_211.c,v 1.4 2022/06/17 18:54:53 rillig Exp $	*/
# 3 "msg_211.c"

// Test for message: return value type mismatch (%s) and (%s) [211]

struct str {
	int member;
};

int
return_int(double dbl, void *ptr, struct str str)
{
	if (dbl > 0.0)
		return dbl;
	if (ptr != (void *)0)
		/* expect+1: warning: illegal combination of integer (int) and pointer (pointer to void) [183] */
		return ptr;
	if (str.member > 0)
		/* expect+1: error: return value type mismatch (int) and (struct str) [211] */
		return str;
	return 3;
}
