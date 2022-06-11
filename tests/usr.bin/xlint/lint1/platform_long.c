/*	$NetBSD: platform_long.c,v 1.4 2022/06/11 11:52:13 rillig Exp $	*/
# 3 "platform_long.c"

/*
 * Test features that only apply to platforms on which size_t is unsigned
 * long and ptr_diff is signed long.
 */

/* lint1-extra-flags: -c -h -a -p -b -r -z */
/* lint1-only-if: long */

void to_size(typeof(sizeof(int)));

/* See should_warn_about_prototype_conversion. */
void
convert_unsigned_char_to_size(unsigned char uc)
{
	/*
	 * In this function call, uc is first promoted to INT. It is then
	 * converted to size_t, which is ULONG. The portable bit size of INT
	 * is 24 (see INT_RSIZE in inittyp.c), which is less than the 32 of
	 * ULONG. Since the portable bit size increases from 24 to 32, there
	 * is no warning.
	 *
	 * XXX: Investigate whether this rule makes sense. Warning 259 is
	 * about prototype mismatch, not about lossy integer conversions,
	 * and there is a clear mismatch here between INT and LONG,
	 * therefore a warning makes sense.
	 */
	to_size(uc);
}

/* expect+1: warning: static variable 'unused_variable' unused [226] */
static int unused_variable;
