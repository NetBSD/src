/*	$NetBSD: platform_ilp32.c,v 1.3 2023/02/22 22:12:35 rillig Exp $	*/
# 3 "platform_ilp32.c"

/*
 * Test features that only apply to platforms that have 32-bit int, long and
 * pointer types.
 */

/* lint1-extra-flags: -c -h -a -p -b -r -z */
/* lint1-only-if: ilp32 */

int s32;
unsigned int u32;
long sl32;
unsigned long ul32;

void
convert_between_int_and_long(void)
{
	/*
	 * No warning about possible loss of accuracy, as the types have the
	 * same size.
	 */
	s32 = sl32;
	sl32 = s32;
	u32 = ul32;
	ul32 = u32;
}
