/*	$NetBSD: msg_259_c90.c,v 1.4 2022/06/17 18:54:53 rillig Exp $	*/
# 3 "msg_259_c90.c"

/* Test for message: argument #%d is converted from '%s' to '%s' due to prototype [259] */

/*
 * This warning detects function calls that are translated in very different
 * translation environments, one where prototypes are omitted, and one where
 * prototypes are active.  It is possible to make such code interoperable,
 * but that requires that each argument is converted to its proper type by
 * the caller of the function.
 *
 * When lint is run with the '-s' flag, it no longer warns about code with
 * incompatibilities between traditional C and C90, therefore this test omits
 * all of the options '-t', '-s', '-S' and '-Ac11'.
 *
 * See also msg_297, which is about lossy integer conversions, but that
 * requires the flags -a -p -P, which are not enabled in the default NetBSD
 * build.
 */

/* lint1-only-if: lp64 */
/* lint1-flags: -h -w */

void plain_char(char);
void signed_char(signed char);
void unsigned_char(unsigned char);
void signed_short(signed short);
void unsigned_short(unsigned short);
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
	/* expect+1: ... from 'long' to 'int' due to prototype [259] */
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
small_integer_types(char c, signed char sc, unsigned char uc,
		    signed short ss, unsigned short us,
		    signed int si, unsigned int ui,
		    signed long sl, unsigned long ul)
{
	plain_char(c);
	plain_char(sc);
	plain_char(uc);
	plain_char(ss);
	plain_char(us);
	plain_char(si);
	plain_char(ui);
	plain_char(sl);
	plain_char(ul);

	signed_char(c);
	signed_char(sc);
	signed_char(uc);
	signed_char(ss);
	signed_char(us);
	signed_char(si);
	signed_char(ui);
	signed_char(sl);
	signed_char(ul);

	unsigned_char(c);
	unsigned_char(sc);
	unsigned_char(uc);
	unsigned_char(ss);
	unsigned_char(us);
	unsigned_char(si);
	unsigned_char(ui);
	unsigned_char(sl);
	unsigned_char(ul);

	signed_short(c);
	signed_short(sc);
	signed_short(uc);
	signed_short(ss);
	signed_short(us);
	signed_short(si);
	signed_short(ui);
	signed_short(sl);
	signed_short(ul);

	unsigned_short(c);
	unsigned_short(sc);
	unsigned_short(uc);
	unsigned_short(ss);
	unsigned_short(us);
	unsigned_short(si);
	unsigned_short(ui);
	unsigned_short(sl);
	unsigned_short(ul);
}

/*
 * This function tests, among others, the conversion from a signed integer
 * type to its corresponding unsigned integer type.  Warning 259 is not
 * about lossy integer conversions but about ABI calling conventions.
 *
 * A common case where a conversion from a signed integer type to its
 * corresponding unsigned integer type occurs is when the difference of two
 * pointers is converted to size_t.  The type ptrdiff_t is defined to be
 * signed, but in many practical cases, the expression is '(end - start)',
 * which makes the resulting value necessarily positive.
 */
void
signed_to_unsigned(int si, long sl)
{
	/* expect+1: warning: argument #1 is converted from 'int' to 'unsigned int' due to prototype [259] */
	unsigned_int(si);

	/* expect+1: warning: argument #1 is converted from 'long' to 'unsigned int' due to prototype [259] */
	unsigned_int(sl);

	/*
	 * No warning here.  Even though 'unsigned long' is 64 bits wide, it
	 * cannot represent negative 32-bit values.  This lossy conversion is
	 * covered by message 297 instead, which requires nonstandard flags.
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
	 * Even though the expression has type size_t, it has a constant
	 * value that fits effortless into an 'unsigned int', it's so small
	 * that it would even fit into a 3-bit bit-field, so lint's warning
	 * may seem wrong here.
	 *
	 * This warning 259 is not about lossy integer conversion though but
	 * instead covers calling conventions that may differ between integer
	 * types of different sizes, and from that point of view, the
	 * constant, even though its value would fit in an unsigned int, is
	 * still passed as size_t.
	 */
	/* expect+1: warning: argument #1 is converted from 'unsigned long' to 'unsigned int' due to prototype [259] */
	unsigned_int(sizeof(int));
}
