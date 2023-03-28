/*	$NetBSD: platform_ilp32_int.c,v 1.2 2023/03/28 14:44:35 rillig Exp $	*/
# 3 "platform_ilp32_int.c"

/*
 * Test features that only apply to platforms that have 32-bit int, long and
 * pointer types and where size_t is unsigned int, not unsigned long.
 */

/* lint1-only-if: ilp32 int */
/* lint1-extra-flags: -c -h -a -p -b -r -z -X 351 */

int s32;
unsigned int u32;
long sl32;
unsigned long ul32;

void
convert_between_int_and_long(void)
{
	/*
	 * No warning about possible loss of accuracy, as the types have the
	 * same size, both in target platform mode as well as in portable
	 * mode.
	 */
	s32 = sl32;
	sl32 = s32;
	u32 = ul32;
	ul32 = u32;
}
