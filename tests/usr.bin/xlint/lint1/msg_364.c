/*	$NetBSD: msg_364.c,v 1.1 2024/03/01 19:39:28 rillig Exp $	*/
# 3 "msg_364.c"

// Test for message: missing bit position after '%.*s' [364]

/*
 * The directives 'b', 'f' and 'F' require a bit position as their first
 * argument.
 */

/* lint1-extra-flags: -X 351 */

typedef typeof(sizeof(0)) size_t;
typedef unsigned long long uint64_t;

int snprintb(char*, size_t, const char*, uint64_t);

void
example(unsigned u32)
{
	char buf[64];

	/* expect+4: warning: missing bit position after 'b' [364] */
	snprintb(buf, sizeof(buf),
	    "\177\020"
	    "b",
	    u32);

	/* expect+4: warning: missing '\0' at the end of 'b\007' [366] */
	snprintb(buf, sizeof(buf),
	    "\177\020"
	    "b\007",
	    u32);
}
