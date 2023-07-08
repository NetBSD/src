/*	$NetBSD: platform_ilp32_long.c,v 1.4 2023/07/08 15:26:25 rillig Exp $	*/
# 3 "platform_ilp32_long.c"

/*
 * Test features that only apply to platforms that have 32-bit int, long and
 * pointer types and where size_t is unsigned long, not unsigned int.
 *
 * On these platforms, in portable mode (-p), the type 'int' is in some cases
 * assumed to be only 24 bits wide, in order to detect conversions from
 * 'long' (or more probably 'size_t') to 'int', which can lose accuracy.
 */

/* lint1-only-if: ilp32 long */
/* lint1-extra-flags: -c -h -a -p -b -r -z -X 351 */

int s32;
unsigned int u32;
long sl32;
unsigned long ul32;

void
convert_between_int_and_long(void)
{
	/*
	 * The '-p' option enables checks that apply independently of the
	 * current platform, assuming that 'long' is always wider than 'int'.
	 * This assumption, when applied on its own, leads to wrong warnings
	 * that a 32-bit 'long' may lose accuracy when converted to a 32-bit
	 * 'int'.
	 *
	 * To avoid these, take a look at the actually possible values of the
	 * right-hand side, and if they fit in the left-hand side, don't warn.
	 */
	s32 = sl32;
	sl32 = s32;
	u32 = ul32;
	ul32 = u32;
}
