/*	$NetBSD: msg_259.c,v 1.9 2021/07/04 17:32:24 rillig Exp $	*/
# 3 "msg_259.c"

// Test for message: argument #%d is converted from '%s' to '%s' due to prototype [259]

/* lint1-only-if lp64 */
/* lint1-extra-flags: -h */

void farg_char(char);
void farg_int(int);
void farg_long(long);

void
example(char c, int i, long l)
{
	farg_char(c);
	farg_int(c);
	/* No warning 259 on LP64, only on ILP32 */
	farg_long(c);

	farg_char(i);		/* XXX: why no warning? */
	farg_int(i);
	/* No warning 259 on LP64, only on ILP32 */
	farg_long(i);

	farg_char(l);		/* XXX: why no warning? */
	/* expect+1: from 'long' to 'int' due to prototype [259] */
	farg_int(l);
	farg_long(l);
}
