/*	$NetBSD: msg_374.c,v 1.4 2024/03/03 13:09:23 rillig Exp $	*/
# 3 "msg_374.c"

// Test for message: unknown directive '%.*s', must be one of 'bfF=:*' [374]

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

	/* expect+4: warning: unknown directive 'x', must be one of 'bfF=:*' [374] */
	snprintb(buf, sizeof(buf),
	    "\177\020"
	    "x12345\0",
	    u64);

	/* expect+4: warning: unknown directive '\000', must be one of 'bfF=:*' [374] */
	snprintb(buf, sizeof(buf),
	    "\177\020"
	    "\00012345\0",
	    u64);

	/* expect+5: warning: redundant '\0' at the end of the format [377] */
	snprintb(buf, sizeof(buf),
	    "\177\020"
	    "b\00012345\0"
	    "\0",
	    u64);

	// Real-life example: the '\b' is a typo.
	//
	// TODO: Warn about the description that is split between string
	//  literals for no apparent reason.
	//
	/* expect+4: warning: unknown directive '\b', must be one of 'bfF=:*' [374] */
	snprintb(buf, sizeof(buf),
	    "\177\020"
	    "b\15ENCNT\0b\16" "TC\0\b\20DSBL_" "CSR_DRN\0",
	    u64);
}
