/*	$NetBSD: msg_359.c,v 1.2 2024/08/31 06:57:31 rillig Exp $	*/
# 3 "msg_359.c"

// Test for message: missing new-style '\177' or old-style number base [359]

/*
 * The first or second character of the snprintb format specifies the number
 * base.  It must be given in binary.
 */

/* lint1-extra-flags: -X 351 */

typedef typeof(sizeof(0)) size_t;
typedef unsigned long long uint64_t;

int snprintb(char *, size_t, const char *, uint64_t);

void
old_style_number_base(void)
{
	char buf[64];

	/* expect+1: warning: missing new-style '\177' or old-style number base [359] */
	snprintb(buf, sizeof(buf), "", 0);
	snprintb(buf, sizeof(buf), "\010", 0);
	snprintb(buf, sizeof(buf), "" "\177\020" "", 0);
}
