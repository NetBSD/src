/*	$NetBSD: msg_370.c,v 1.3 2024/08/31 06:57:31 rillig Exp $	*/
# 3 "msg_370.c"

// Test for message: field width '%.*s' in '%.*s' should be escaped as octal or hex [370]

/*
 * To distinguish field widths from the description text, they should use
 * octal or hex escape sequences.  Of these, octal escape sequences are less
 * error-prone, as they consist of at most 3 octal digits, whereas hex escape
 * sequences consume as many digits as available.
 */

/* lint1-extra-flags: -X 351 */

typedef typeof(sizeof(0)) size_t;
typedef unsigned long long uint64_t;

int snprintb(char *, size_t, const char *, uint64_t);

void
example(uint64_t u64)
{
	char buf[64];

	/* expect+11: warning: bit position ' ' in 'f  space\0' should be escaped as octal or hex [369] */
	/* expect+10: warning: field width ' ' in 'f  space\0' should be escaped as octal or hex [370] */
	/* expect+9: warning: bit position '\t' in 'f\t\ttab\0' should be escaped as octal or hex [369] */
	/* expect+8: warning: field width '\t' in 'f\t\ttab\0' should be escaped as octal or hex [370] */
	/* expect+7: warning: bit position '\n' in 'f\n\nnewline\0' should be escaped as octal or hex [369] */
	/* expect+6: warning: field width '\n' in 'f\n\nnewline\0' should be escaped as octal or hex [370] */
	snprintb(buf, sizeof(buf),
	    "\177\020"
	    "f  space\0"
	    "f\t\ttab\0"
	    "f\n\nnewline\0",
	    u64);
	/* expect-1: warning: 'f\n\nnewline\0' overlaps earlier 'f\t\ttab\0' on bit 10 [376] */

	/* expect+11: warning: bit position ' ' in 'F  space\0' should be escaped as octal or hex [369] */
	/* expect+10: warning: field width ' ' in 'F  space\0' should be escaped as octal or hex [370] */
	/* expect+9: warning: bit position '\t' in 'F\t\ttab\0' should be escaped as octal or hex [369] */
	/* expect+8: warning: field width '\t' in 'F\t\ttab\0' should be escaped as octal or hex [370] */
	/* expect+7: warning: bit position '\n' in 'F\n\nnewline\0' should be escaped as octal or hex [369] */
	/* expect+6: warning: field width '\n' in 'F\n\nnewline\0' should be escaped as octal or hex [370] */
	snprintb(buf, sizeof(buf),
	    "\177\020"
	    "F  space\0"
	    "F\t\ttab\0"
	    "F\n\nnewline\0",
	    u64);
	/* expect-1: warning: 'F\n\nnewline\0' overlaps earlier 'F\t\ttab\0' on bit 10 [376] */
}
