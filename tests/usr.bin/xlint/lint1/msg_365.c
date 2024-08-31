/*	$NetBSD: msg_365.c,v 1.4 2024/08/31 06:57:31 rillig Exp $	*/
# 3 "msg_365.c"

// Test for message: missing field width after '%.*s' [365]

/*
 * The conversions 'f' and 'F' require a field width as their second argument.
 */

/* lint1-extra-flags: -X 351 */

typedef typeof(sizeof(0)) size_t;
typedef unsigned long long uint64_t;

int snprintb(char *, size_t, const char *, uint64_t);

void
example(unsigned u32)
{
	char buf[64];

	/* expect+4: warning: missing field width after 'f\000' [365] */
	snprintb(buf, sizeof(buf),
	    "\177\020"
	    "f\000",
	    u32);

	/* expect+5: warning: empty description in 'f\007\010' [367] */
	/* expect+4: warning: missing '\0' at the end of 'f\007\010' [366] */
	snprintb(buf, sizeof(buf),
	    "\177\020"
	    "f\007\010",
	    u32);
}
