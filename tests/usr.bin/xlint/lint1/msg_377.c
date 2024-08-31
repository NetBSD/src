/*	$NetBSD: msg_377.c,v 1.5 2024/08/31 06:57:32 rillig Exp $	*/
# 3 "msg_377.c"

// Test for message: redundant '\0' at the end of the format [377]

/*
 * Each conversion in the new-style format ends with a '\0' that needs to be
 * spelled out.
 *
 * In both old-style and new-style formats, the '\0' that ends the whole
 * format is provided by the compiler as part of the string literal.
 */

/* lint1-extra-flags: -X 351 */

typedef typeof(sizeof(0)) size_t;
typedef unsigned long long uint64_t;

int snprintb(char *, size_t, const char *, uint64_t);

void
example(unsigned u32, uint64_t u64)
{
	char buf[64];

	/* expect+7: warning: bit position '\000' (0) in '\000out-of-range' out of range 1..32 [371] */
	/* expect+6: warning: redundant '\0' at the end of the format [377] */
	snprintb(buf, sizeof(buf),
	    "\020"
	    "\005bit"
	    "\000out-of-range"
	    "\0",
	    u32);

	/* expect+5: warning: redundant '\0' at the end of the format [377] */
	snprintb(buf, sizeof(buf),
	    "\177\020"
	    "b\005bit\0"
	    "\0",
	    u64);
}
