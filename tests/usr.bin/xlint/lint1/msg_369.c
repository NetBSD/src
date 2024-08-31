/*	$NetBSD: msg_369.c,v 1.3 2024/08/31 06:57:31 rillig Exp $	*/
# 3 "msg_369.c"

// Test for message: bit position '%.*s' in '%.*s' should be escaped as octal or hex [369]

/*
 * To distinguish bit positions from the description text, they should use
 * octal or hex escape sequences.  Of these, octal escape sequences are less
 * error-prone, as they consist of at most 3 octal digits, whereas hex escape
 * sequences consume as many digits as available.
 */

/* lint1-extra-flags: -X 351 */

typedef typeof(sizeof(0)) size_t;
typedef unsigned long long uint64_t;

int snprintb(char *, size_t, const char *, uint64_t);

void
example(unsigned u32, uint64_t u64)
{
	char buf[64];

	/* expect+8: warning: bit position ' ' in ' space' should be escaped as octal or hex [369] */
	/* expect+7: warning: bit position '\t' in '\ttab' should be escaped as octal or hex [369] */
	/* expect+6: warning: bit position '\n' in '\nnewline' should be escaped as octal or hex [369] */
	snprintb(buf, sizeof(buf),
	    "\020"
	    " space"
	    "\ttab"
	    "\nnewline",
	    u32);

	/* expect+8: warning: bit position ' ' in 'b space\0' should be escaped as octal or hex [369] */
	/* expect+7: warning: bit position '\t' in 'b\ttab\0' should be escaped as octal or hex [369] */
	/* expect+6: warning: bit position '\n' in 'b\nnewline\0' should be escaped as octal or hex [369] */
	snprintb(buf, sizeof(buf),
	    "\177\020"
	    "b space\0"
	    "b\ttab\0"
	    "b\nnewline\0",
	    u64);

	/* expect+6: warning: bit position '\t' in 'f\t\001tab\0' should be escaped as octal or hex [369] */
	/* expect+5: warning: bit position '\n' in 'F\n\001newline\0' should be escaped as octal or hex [369] */
	snprintb(buf, sizeof(buf),
	    "\177\020"
	    "f\t\001tab\0"
	    "F\n\001newline\0",
	    u64);
}
