/*	$NetBSD: msg_360.c,v 1.3 2024/08/31 06:57:31 rillig Exp $	*/
# 3 "msg_360.c"

// Test for message: missing new-style number base after '\177' [360]

/*
 * The new-style format requires the number base as the second character.
 * This check is merely a byproduct of the implementation, it does not provide
 * much value on its own.
 */

/* lint1-extra-flags: -X 351 */

typedef typeof(sizeof(0)) size_t;
typedef unsigned long long uint64_t;

int snprintb(char *, size_t, const char *, uint64_t);

void
new_style_number_base(void)
{
	char buf[64];

	/* expect+1: warning: missing new-style number base after '\177' [360] */
	snprintb(buf, sizeof(buf), "\177", 0);
	/* expect+1: warning: number base '\002' is 2, must be 8, 10 or 16 [361] */
	snprintb(buf, sizeof(buf), "\177\002", 0);
}
