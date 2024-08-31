/*	$NetBSD: msg_374.c,v 1.6 2024/08/31 06:57:31 rillig Exp $	*/
# 3 "msg_374.c"

// Test for message: unknown conversion '%.*s', must be one of 'bfF=:*' [374]

/*
 * In the new-style format, an unknown conversion is assumed to have a single
 * argument, followed by a null-terminated description.
 */

/* lint1-extra-flags: -X 351 */

typedef typeof(sizeof(0)) size_t;
typedef unsigned long long uint64_t;

int snprintb(char *, size_t, const char *, uint64_t);

void
example(uint64_t u64)
{
	char buf[64];

	/* expect+4: warning: unknown conversion 'x', must be one of 'bfF=:*' [374] */
	snprintb(buf, sizeof(buf),
	    "\177\020"
	    "x12345\0",
	    u64);

	/* expect+4: warning: unknown conversion '\000', must be one of 'bfF=:*' [374] */
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
	/* expect+4: warning: unknown conversion '\b', must be one of 'bfF=:*' [374] */
	snprintb(buf, sizeof(buf),
	    "\177\020"
	    "b\15ENCNT\0b\16" "TC\0\b\20DSBL_CSR_DRN\0",
	    u64);
}
