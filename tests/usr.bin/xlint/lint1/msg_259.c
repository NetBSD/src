/*	$NetBSD: msg_259.c,v 1.12 2021/08/21 11:58:12 rillig Exp $	*/
# 3 "msg_259.c"

// Test for message: argument #%d is converted from '%s' to '%s' due to prototype [259]

/* lint1-only-if: lp64 */
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

void farg_unsigned_int(unsigned int);
void farg_unsigned_long(unsigned long);
void farg_unsigned_long_long(unsigned long long);

/*
 * Converting a signed integer type to its corresponding unsigned integer
 * type (C99 6.2.5p6) is usually not a problem.  A common case where it
 * occurs is when the difference of two pointers is converted to size_t.
 */
void
convert_to_corresponding_unsigned(int i, long l, long long ll)
{
	/* TODO: don't warn here. */
	/* expect+1: warning: argument #1 is converted from 'int' to 'unsigned int' due to prototype [259] */
	farg_unsigned_int(i);

	/* TODO: don't warn here. */
	/* expect+1: warning: argument #1 is converted from 'long' to 'unsigned long' due to prototype [259] */
	farg_unsigned_long(l);

	/* TODO: don't warn here. */
	/* expect+1: warning: argument #1 is converted from 'long long' to 'unsigned long long' due to prototype [259] */
	farg_unsigned_long_long(ll);

	/*
	 * XXX: Why no warning?  Even though 'unsigned long' is 64 bits
	 * wide, it cannot represent negative 32-bit values.
	 */
	farg_unsigned_long(i);

	/*
	 * XXX: Why no warning?  Even though 'unsigned long long' is 64 bits
	 * wide, it cannot represent negative 32-bit values.
	 */
	farg_unsigned_long_long(i);

	/* expect+1: warning: argument #1 is converted from 'long' to 'unsigned long long' due to prototype [259] */
	farg_unsigned_long_long(l);
}
