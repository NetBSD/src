/*	$NetBSD: msg_359.c,v 1.3 2024/11/05 06:23:04 rillig Exp $	*/
# 3 "msg_359.c"

// Test for message: missing new-style '\177' or old-style number base [359]

/*
 * The first or second character of the snprintb format specifies the number
 * base.  It must be an octal or hexadecimal escape sequence, as the characters
 * 2, 10 and 16 are not printable, and writing '\n' instead of '\x0a' would be
 * misleading.
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
