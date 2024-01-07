/*	$NetBSD: msg_107.c,v 1.7 2024/01/07 21:19:42 rillig Exp $	*/
# 3 "msg_107.c"

// Test for message: operands of '%s' have incompatible types '%s' and '%s' [107]

/* lint1-extra-flags: -X 351 */

/* ARGSUSED */
void
compare(double d, void *ptr)
{
	/* expect+1: error: operands of '==' have incompatible types 'double' and 'pointer to void' [107] */
	return d == ptr;
}

static inline void
typeok_minus(const char *p, int i, double d)
{
	if (p[i] != p[(int)d])
		return;
	/* expect+1: error: operands of '-' have incompatible types 'pointer to const char' and 'double' [107] */
	return (p - i) - (p - d);
}
