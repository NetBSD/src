/*	$NetBSD: msg_259_c90.c,v 1.1 2021/08/31 18:59:26 rillig Exp $	*/
# 3 "msg_259_c90.c"

/* Test for message: argument #%d is converted from '%s' to '%s' due to prototype [259] */

/* lint1-only-if: lp64 */
/* XXX: The flag '-s' suppresses all warnings.  Why? */
/* lint1-flags: -h -w */

void plain_char(char);
void signed_int(int);
void unsigned_int(unsigned int);
void signed_long(long);
void unsigned_long(unsigned long);
/* No 'long long' since it requires C99. */

void
change_in_type_width(char c, int i, long l)
{
	plain_char(c);
	signed_int(c);
	/* No warning 259 on LP64, only on ILP32 */
	signed_long(c);

	plain_char(i);		/* XXX: why no warning? */
	signed_int(i);
	/* No warning 259 on LP64, only on ILP32 */
	signed_long(i);

	plain_char(l);		/* XXX: why no warning? */
	/* expect+1: from 'long' to 'int' due to prototype [259] */
	signed_int(l);
	signed_long(l);
}

/*
 * Converting a signed integer type to its corresponding unsigned integer
 * type (C99 6.2.5p6) is usually not a problem since the actual values of the
 * expressions are usually not anywhere near the maximum signed value.  From
 * a technical standpoint, it is correct to warn here since even small
 * negative numbers may result in very large positive numbers.
 *
 * A common case where it occurs is when the difference of two pointers is
 * converted to size_t.  The type ptrdiff_t is defined to be signed, but in
 * many practical cases, the expression is '(end - start)', which makes the
 * resulting value necessarily positive.
 */
void
signed_to_unsigned(int si, long sl)
{
	/* expect+1: warning: argument #1 is converted from 'int' to 'unsigned int' due to prototype [259] */
	unsigned_int(si);

	/* expect+1: warning: argument #1 is converted from 'long' to 'unsigned int' due to prototype [259] */
	unsigned_int(sl);

	/*
	 * XXX: Why no warning?  Even though 'unsigned long' is 64 bits
	 * wide, it cannot represent negative 32-bit values.
	 */
	unsigned_long(si);

	/* expect+1: warning: argument #1 is converted from 'long' to 'unsigned long' due to prototype [259] */
	unsigned_long(sl);
}

void
unsigned_to_signed(unsigned int ui, unsigned long ul)
{
	/* expect+1: warning: argument #1 is converted from 'unsigned int' to 'int' due to prototype [259] */
	signed_int(ui);
	/* expect+1: warning: argument #1 is converted from 'unsigned long' to 'int' due to prototype [259] */
	signed_int(ul);
	signed_long(ui);
	/* expect+1: warning: argument #1 is converted from 'unsigned long' to 'long' due to prototype [259] */
	signed_long(ul);
}

void
signed_to_signed(signed int si, signed long sl)
{
	signed_int(si);
	/* expect+1: warning: argument #1 is converted from 'long' to 'int' due to prototype [259] */
	signed_int(sl);
	signed_long(si);
	signed_long(sl);
}

void
unsigned_to_unsigned(unsigned int ui, unsigned long ul)
{
	unsigned_int(ui);
	/* expect+1: warning: argument #1 is converted from 'unsigned long' to 'unsigned int' due to prototype [259] */
	unsigned_int(ul);
	unsigned_long(ui);
	unsigned_long(ul);
}

void
pass_sizeof_as_smaller_type(void)
{
	/*
	 * XXX: Even though the expression has type size_t, it has a constant
	 * value that fits effortless into an 'unsigned int', it's so small
	 * that it would even fit into a 3-bit bit-field, so lint should not
	 * warn here.
	 */
	/* expect+1: warning: argument #1 is converted from 'unsigned long' to 'unsigned int' due to prototype [259] */
	unsigned_int(sizeof(int));
}
