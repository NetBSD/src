/*	$NetBSD: msg_118_ilp32.c,v 1.1 2025/09/06 20:18:41 rillig Exp $	*/
# 3 "msg_118_ilp32.c"

/* Test for message: '%s' %s '%s' differs between traditional C and C90 [118] */

/* lint1-flags: -hw -X 351 */
/* lint1-only-if: ilp32 */

int si;
unsigned ui;
long sl;
unsigned long ul;

/*
 * On 32-bit platforms both operands of the '<<' operator are first promoted
 * individually, and since C90 does not know 'long long', the maximum
 * bit-size for an integer type is 32 bits.
 */
void
test_shl(void)
{
	si <<= si;
	si <<= ui;
	si <<= sl;
	si <<= ul;

	si = si << si;
	si = si << ui;
	si = si << sl;
	si = si << ul;
}

void
test_shr(void)
{
	si >>= si;
	si >>= ui;
	si >>= sl;
	si >>= ul;

	si = si >> si;
	/* expect+1: warning: 'int' >> 'unsigned int' differs between traditional C and C90 [118] */
	si = si >> ui;
	si = si >> sl;
	/* expect+1: warning: 'int' >> 'unsigned long' differs between traditional C and C90 [118] */
	si = si >> ul;
}
