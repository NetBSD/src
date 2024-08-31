/*	$NetBSD: msg_366.c,v 1.5 2024/08/31 06:57:31 rillig Exp $	*/
# 3 "msg_366.c"

// Test for message: missing '\0' at the end of '%.*s' [366]

/*
 * In the new-style format, each conversion ends with a '\0'.  If that's not
 * the case, snprintb will read beyond the end of the format argument, looking
 * for the terminating '\0'.  In the most common case where the format comes
 * from a string literal, the '\0' from the conversion needs to be spelled
 * out, while the '\0' that terminates the sequence of conversions is provided
 * by the C compiler.
 */

/* lint1-extra-flags: -X 351 */

typedef typeof(sizeof(0)) size_t;
typedef unsigned long long uint64_t;

int snprintb(char *, size_t, const char *, uint64_t);

void
example(unsigned u32)
{
	char buf[64];

	/* expect+4: warning: redundant '\0' at the end of the format [377] */
	snprintb(buf, sizeof(buf),
	    "\177\020"
	    "\0",
	    u32);

	/* expect+5: warning: empty description in 'b\007' [367] */
	/* expect+4: warning: missing '\0' at the end of 'b\007' [366] */
	snprintb(buf, sizeof(buf),
	    "\177\020"
	    "b\007",
	    u32);

	/* expect+5: warning: empty description in 'f\007\000' [367] */
	/* expect+4: warning: missing '\0' at the end of 'f\007\000' [366] */
	snprintb(buf, sizeof(buf),
	    "\177\020"
	    "f\007\000",
	    u32);

	/* expect+4: warning: missing '\0' at the end of 'F\007\000' [366] */
	snprintb(buf, sizeof(buf),
	    "\177\020"
	    "F\007\000",
	    u32);

	/* expect+4: warning: missing '\0' at the end of '=\007value' [366] */
	snprintb(buf, sizeof(buf),
	    "\177\020"
	    "=\007value",
	    u32);

	/* expect+4: warning: missing '\0' at the end of ':\007value' [366] */
	snprintb(buf, sizeof(buf),
	    "\177\020"
	    ":\007value",
	    u32);

	/* expect+4: warning: missing '\0' at the end of '*default' [366] */
	snprintb(buf, sizeof(buf),
	    "\177\020"
	    "*default",
	    u32);
}
