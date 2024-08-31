/*	$NetBSD: msg_357.c,v 1.2 2024/08/31 06:57:31 rillig Exp $	*/
# 3 "msg_357.c"

// Test for message: hex escape '%.*s' mixes uppercase and lowercase digits [357]

/*
 * In the format argument of the snprintb and snprintb_m functions, a bit
 * position or field width is written as an octal or hexadecimal escape
 * sequence.  If the description that follows starts with hex digits (A-Fa-f),
 * these digits are still part of the escape sequence instead of the
 * description.
 *
 * Since the escape sequences are typically written in lowercase and the
 * descriptions are typically written in uppercase, a mixture of both cases
 * indicates a mismatch.
 */

/* lint1-extra-flags: -X 351 */

typedef typeof(sizeof(0)) size_t;
typedef unsigned long long uint64_t;

int snprintb(char *, size_t, const char *, uint64_t);

void
examples(unsigned u32, uint64_t u64)
{
	char buf[64];

	/* expect+5: warning: hex escape '\x0aB' mixes uppercase and lowercase digits [357] */
	/* expect+4: warning: hex escape '\x0aB' has more than 2 digits [358] */
	/* expect+3: warning: bit position '\x0aB' (171) in '\x0aBIT' out of range 1..32 [371] */
	snprintb(buf, sizeof(buf),
	    "\020\x0aBIT",
	    u32);

	// This mismatch goes undetected as it has only 2 digits, does not mix
	// case and is in bounds.  A spellchecker could mark the unknown word
	// 'ield' to give a hint.
	snprintb(buf, sizeof(buf),
	    "\020\x1FIELD",
	    u32);

	/* expect+5: warning: hex escape '\x0aB' mixes uppercase and lowercase digits [357] */
	/* expect+4: warning: hex escape '\x0aB' has more than 2 digits [358] */
	/* expect+3: warning: bit position '\x0aB' (171) in 'b\x0aBIT\0' out of range 0..63 [371] */
	snprintb(buf, sizeof(buf),
	    "\177\020b\x0aBIT\0",
	    u64);
}
