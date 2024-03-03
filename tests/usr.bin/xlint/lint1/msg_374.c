/*	$NetBSD: msg_374.c,v 1.2 2024/03/03 00:50:41 rillig Exp $	*/
# 3 "msg_374.c"

// Test for message: unknown directive '%.*s' [374]

/*
 * In the new-style format, an unknown directive is assumed to have a single
 * argument, followed by a null-terminated description.
 */

/* lint1-extra-flags: -X 351 */

typedef typeof(sizeof(0)) size_t;
typedef unsigned long long uint64_t;

int snprintb(char*, size_t, const char*, uint64_t);

void
example(uint64_t u64)
{
	char buf[64];

	/* expect+4: warning: unknown directive 'x' [374] */
	snprintb(buf, sizeof(buf),
	    "\177\020"
	    "x12345\0",
	    u64);

	/* expect+4: warning: unknown directive '\000' [374] */
	snprintb(buf, sizeof(buf),
	    "\177\020"
	    "\00012345\0",
	    u64);

	/* expect+5: warning: redundant '\0' at the end of new-style format [377] */
	snprintb(buf, sizeof(buf),
	    "\177\020"
	    "b\00012345\0"
	    "\0",
	    u64);
}
