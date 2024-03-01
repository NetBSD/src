/*	$NetBSD: msg_358.c,v 1.1 2024/03/01 19:39:28 rillig Exp $	*/
# 3 "msg_358.c"

// Test for message: hex escape '%.*s' has more than 2 digits [358]

/*
 * In the format argument of the snprintb and snprintb_m functions, a bit
 * position or field width is written as an octal or hexadecimal escape
 * sequence.  If the description that follows starts with hex digits (A-Fa-f),
 * these digits are still part of the escape sequence instead of the
 * description.
 */

/* lint1-extra-flags: -X 351 */

typedef typeof(sizeof(0)) size_t;
typedef unsigned long long uint64_t;

int snprintb(char*, size_t, const char*, uint64_t);

void
examples(unsigned u32, uint64_t u64)
{
	char buf[64];

	/* expect+3: warning: hex escape '\x01B' has more than 2 digits [358] */
	snprintb(buf, sizeof(buf),
	    "\020\x01BIT",
	    u32);

	/* expect+3: warning: hex escape '\x01b' has more than 2 digits [358] */
	snprintb(buf, sizeof(buf),
	    "\020\x01bit",
	    u32);

	// This mismatch goes undetected as it has only 2 digits, does not mix
	// case and is in bounds.  A spellchecker could mark the unknown word
	// 'ield' to give a hint.
	snprintb(buf, sizeof(buf),
	    "\020\x1FIELD",
	    u32);

	/* expect+3: warning: hex escape '\x01b' has more than 2 digits [358] */
	snprintb(buf, sizeof(buf),
	    "\177\020b\x01bit\0",
	    u64);

	/* expect+3: warning: hex escape '\x02b' has more than 2 digits [358] */
	snprintb(buf, sizeof(buf),
	    "\177\020f\x00\x02bit\0",
	    u64);
}
