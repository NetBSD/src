/*	$NetBSD: msg_380.c,v 1.2 2024/06/22 06:24:46 rillig Exp $	*/
# 3 "msg_380.c"

// Test for message: lossy conversion of %Lg to '%s', arg #%d [380]

/* lint1-extra-flags: -X 351 */

void take_s32(int);
void take_u32(unsigned int);
void take_s64(long long);
void take_u64(unsigned long long);

void
conversions(void)
{
	/* expect+1: warning: lossy conversion of -2.14748e+09 to 'int', arg #1 [380] */
	take_s32(-2147483649.0);
	take_s32(-2147483648.0);
	/* expect+1: warning: lossy conversion of 3.141 to 'int', arg #1 [380] */
	take_s32(3.141);
	take_s32(2147483647.0);
	/* expect+1: warning: lossy conversion of 2.14748e+09 to 'int', arg #1 [380] */
	take_s32(2147483648.0);

	/* expect+1: warning: lossy conversion of -1 to 'unsigned int', arg #1 [380] */
	take_u32(-1.0);
	take_u32(-0.0);
	take_u32(0.0);
	/* expect+1: warning: lossy conversion of 3.141 to 'unsigned int', arg #1 [380] */
	take_u32(3.141);
	take_u32(4294967295.0);
	/* expect+1: warning: lossy conversion of 4.29497e+09 to 'unsigned int', arg #1 [380] */
	take_u32(4294967296.0);

	/* expect+1: warning: lossy conversion of -9.22337e+18 to 'long long', arg #1 [380] */
	take_s64(-9223372036854776833.0);
	/* The constant ...809 is rounded down to ...808, thus no warning. */
	take_s64(-9223372036854775809.0);
	take_s64(-9223372036854775808.0);
	/* expect+1: warning: lossy conversion of 3.141 to 'long long', arg #1 [380] */
	take_s64(3.141);
	/* expect+1: warning: lossy conversion of 9.22337e+18 to 'long long', arg #1 [380] */
	take_s64(9223372036854775807.0);
	/* expect+1: warning: lossy conversion of 9.22337e+18 to 'long long', arg #1 [380] */
	take_s64(9223372036854775808.0);

	/* expect+1: warning: lossy conversion of -1 to 'unsigned long long', arg #1 [380] */
	take_u64(-1.0);
	take_u64(-0.0);
	take_u64(0.0);
	/* expect+1: warning: lossy conversion of 3.141 to 'unsigned long long', arg #1 [380] */
	take_u64(3.141);

	// Warning on:		alpha
	// No warning on:	aarch64 aarch64-compat32 arm i386 mips powerpc riscv64 sh3 sparc x86_64
	// Unknown:		coldfire hppa ia64 m68000 m68k mips64 mipsn64 or1k powerpc64 riscv32 sparc64 vax
	//
	// warning: lossy conversion of 1.84467e+19 to 'unsigned long long', arg #1 [380]
	//take_u64(18446744073709550591.0);

	// Warning on:		aarch64 alpha arm i386 mips riscv64 sparc x86_64
	// No warning on:	aarch64-compat32 powerpc sh3
	// Unknown:		coldfire hppa ia64 m68000 m68k mips64 mipsn64 or1k powerpc64 riscv32 sparc64 vax
	//
	// warning: lossy conversion of 1.84467e+19 to 'unsigned long long', arg #1 [380]
	//take_u64(18446744073709550592.0);
	// warning: lossy conversion of 1.84467e+19 to 'unsigned long long', arg #1 [380]
	//take_u64(18446744073709551615.0);
	// warning: lossy conversion of 1.84467e+19 to 'unsigned long long', arg #1 [380]
	//take_u64(18446744073709551616.0);
}
