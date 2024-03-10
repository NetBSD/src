/*	$NetBSD: msg_141.c,v 1.13 2024/03/10 10:31:29 rillig Exp $	*/
# 3 "msg_141.c"

// Test for message: operator '%s' produces integer overflow [141]

/* lint1-extra-flags: -h -X 351 */

// Integer overflow occurs when the arbitrary-precision result of an
// arithmetic operation cannot be represented by the type of the expression.

signed int s32;
unsigned int u32;
signed long long s64;
unsigned long long u64;

void
compl_s32(void)
{
	s32 = ~(-0x7fffffff - 1);
	s32 = ~-1;
	s32 = ~0;
	s32 = ~1;
	s32 = ~0x7fffffff;
}

void
compl_u32(void)
{
	u32 = ~0x00000000U;
	u32 = ~0x7fffffffU;
	u32 = ~0x80000000U;
	u32 = ~0xffffffffU;
}

void
compl_s64(void)
{
	s64 = ~(-0x7fffffffffffffffLL - 1LL);
	s64 = ~-1LL;
	s64 = ~0LL;
	s64 = ~0x7fffffffffffffffLL;
}

void
compl_u64(void)
{
	u64 = ~0ULL;
	u64 = ~0x7fffffffffffffffULL;
	u64 = ~0x8000000000000000ULL;
	u64 = ~0xffffffffffffffffULL;
}

void
uplus_s32(void)
{
	s32 = +(-0x7fffffff - 1);
	s32 = +-1;
	s32 = +0;
	s32 = +0x7fffffff;
}

void
uplus_u32(void)
{
	u32 = +0x00000000U;
	u32 = +0x7fffffffU;
	u32 = +0x80000000U;
	u32 = +0xffffffffU;
}

void
uplus_s64(void)
{
	s64 = +(-0x7fffffffffffffffLL - 1LL);
	s64 = +-1LL;
	s64 = +0LL;
	s64 = +0x7fffffffffffffffLL;
}

void
uplus_u64(void)
{
	u64 = +0x0000000000000000ULL;
	u64 = +0x7fffffffffffffffULL;
	u64 = +0x8000000000000000ULL;
	u64 = +0xffffffffffffffffULL;
}

void
uminus_s32(void)
{
	/* expect+1: warning: operator '-' produces integer overflow [141] */
	s32 = -(-0x7fffffff - 1);
	s32 = - -1;
	s32 = -0;
	s32 = -0x7fffffff;
}

void
uminus_u32(void)
{
	u32 = -0x00000000U;
	u32 = -0x7fffffffU;
	u32 = -0x80000000U;
	u32 = -0xffffffffU;
}

void
uminus_s64(void)
{
	/* expect+1: warning: operator '-' produces integer overflow [141] */
	s64 = -(-0x7fffffffffffffffLL - 1LL);
	s64 = - -1LL;
	s64 = -0LL;
	s64 = -0x7fffffffffffffffLL;
}

void
uminus_u64(void)
{
	u64 = -0x0000000000000000ULL;
	u64 = -0x7fffffffffffffffULL;
	u64 = -0x8000000000000000ULL;
	u64 = -0xffffffffffffffffULL;
}

void
mult_s32(void)
{
	/* expect+1: warning: operator '*' produces integer overflow [141] */
	s32 = -0x00010000 * +0x00010000;	// -0x0100000000
	/* expect+1: warning: operator '*' produces integer overflow [141] */
	s32 = -0x00000003 * +0x2aaaaaab;	// -0x80000001
	/* expect+1: warning: operator '*' produces integer overflow [141] */
	s32 = +0x2aaaaaab * -0x00000003;	// -0x80000001
	s32 = -0x00000008 * +0x10000000;	// -0x80000000
	s32 = +0x10000000 * -0x00000008;	// -0x80000000
	s32 = +0x00000002 * +0x3fffffff;	// +0x7ffffffe
	s32 = +0x3fffffff * +0x00000002;	// +0x7ffffffe
	s32 = +0x7fffffff * +0x00000001;	// +0x7fffffff
	s32 = +0x00000001 * +0x7fffffff;	// +0x7fffffff
	/* expect+1: warning: operator '*' produces integer overflow [141] */
	s32 = +0x00000002 * +0x40000000;	// +0x80000000
	/* expect+1: warning: operator '*' produces integer overflow [141] */
	s32 = +0x40000000 * +0x00000002;	// +0x80000000
	/* expect+1: warning: operator '*' produces integer overflow [141] */
	s32 = +0x0000ffff * +0x00010001;	// +0xffffffff
	/* expect+1: warning: operator '*' produces integer overflow [141] */
	s32 = +0x00010000 * +0x00010000;	// +0x0100000000
}

void
mult_u32(void)
{
	u32 = 0xffffU * 0x10001U;		// +0xffffffff
	/* expect+1: warning: operator '*' produces integer overflow [141] */
	u32 = 0x10000U * 0x10000U;		// +0x0100000000
}

void
mult_s64(void)
{
	/* expect+1: warning: operator '*' produces integer overflow [141] */
	s64 = -0x100000000LL * 0x100000000LL;	// -0x010000000000000000
	/* expect+1: warning: operator '*' produces integer overflow [141] */
	s64 = -3LL * 0x2aaaaaaaaaaaaaabLL;	// -0x8000000000000001
	/* expect+1: warning: operator '*' produces integer overflow [141] */
	s64 = 0x2aaaaaaaaaaaaaabLL * -3LL;	// -0x8000000000000001
	s64 = -8LL * +0x1000000000000000LL;	// -0x8000000000000000
	s64 = +0x1000000000000000LL * -8LL;	// -0x8000000000000000
	s64 = +2LL * +0x3fffffffffffffffLL;	// +0x7ffffffffffffffe
	s64 = +0x3fffffffffffffffLL * +2LL;	// +0x7ffffffffffffffe
	s64 = +0x7fffffffffffffffLL * +1LL;	// +0x7fffffffffffffff
	s64 = +1LL * +0x7fffffffffffffffLL;	// +0x7fffffffffffffff
	/* expect+1: warning: operator '*' produces integer overflow [141] */
	s64 = +2LL * +0x4000000000000000LL;	// +0x8000000000000000
	/* expect+1: warning: operator '*' produces integer overflow [141] */
	s64 = +0x4000000000000000LL * +2LL;	// +0x8000000000000000
	/* expect+1: warning: operator '*' produces integer overflow [141] */
	s64 = +0xffffffffLL * +0x100000001LL;	// +0xffffffffffffffff
	/* expect+1: warning: operator '*' produces integer overflow [141] */
	s64 = +0x100000000LL * +0x100000000LL;	// +0x010000000000000000
}

void
mult_u64(void)
{
	u64 = 0xffffffffULL * 0x100000001ULL;	// +0xffffffffffffffff
	u64 = 0x00010000ULL * 0x00010000ULL;	// +0x0100000000
	/* expect+1: warning: operator '*' produces integer overflow [141] */
	u64 = 0x100000000ULL * 0x100000000ULL;	// +0x010000000000000000
}

void
div_s32(void)
{
	/* expect+1: warning: operator '/' produces integer overflow [141] */
	s32 = (-0x7fffffff - 1) / -1;
	s32 = (-0x7fffffff - 1) / 1;
	s32 = 0x7fffffff / -1;
	/* expect+1: error: division by 0 [139] */
	s32 = 0 / 0;
	/* expect+1: error: division by 0 [139] */
	s32 = 0x7fffffff / 0;
}

void
div_u32(void)
{
	u32 = 0xffffffffU / -1U;
	u32 = 0xffffffffU / 1U;
	/* expect+1: error: division by 0 [139] */
	u32 = 0U / 0U;
	/* expect+1: error: division by 0 [139] */
	u32 = 0xffffffffU / 0U;
}

void
div_s64(void)
{
	/* expect+1: warning: operator '/' produces integer overflow [141] */
	s64 = (-0x7fffffffffffffffLL - 1LL) / -1LL;
	s64 = (-0x7fffffffffffffffLL - 1LL) / 1LL;
	s64 = (-0x7fffffffffffffffLL - 1LL) / 0x7fffffffffffffffLL;
	s64 = 0x7fffffffffffffffLL / -1LL;
	s64 = (-0x7fffffffLL - 1LL) / -1LL;
	s64 = (-0x7fffffffLL - 1LL) / 0x7fffffffLL;
	/* expect+1: error: division by 0 [139] */
	s64 = 0LL / 0LL;
	/* expect+1: error: division by 0 [139] */
	s64 = 0x7fffffffffffffffLL / 0LL;
}

void
div_u64(void)
{
	u64 = 0xffffffffffffffffULL / -1ULL;
	u64 = 0xffffffffffffffffULL / 1ULL;
	/* expect+1: error: division by 0 [139] */
	u64 = 0ULL / 0ULL;
	/* expect+1: error: division by 0 [139] */
	u64 = 0xffffffffffffffffULL / 0ULL;
}

void
mod_s32(void)
{
	s32 = -1 % (-0x7fffffff - 1);
	s32 = -1 % 0x7fffffff;
	/* expect+1: warning: operator '%' produces integer overflow [141] */
	s32 = (-0x7fffffff - 1) % -1;
	s32 = 0x7fffffff % -1;
}

void
mod_u32(void)
{
	u64 = 0xffffffffU % -1U;
	u64 = 0xffffffffU % 1U;
	/* expect+1: error: modulus by 0 [140] */
	u64 = 0U % 0U;
	/* expect+1: error: modulus by 0 [140] */
	u64 = 0xffffffffU % 0U;
}

void
mod_s64(void)
{
	s64 = -1LL % (-0x7fffffffffffffffLL - 1LL);
	s64 = -1LL % 0x7fffffffffffffffLL;
	/* expect+1: warning: operator '%' produces integer overflow [141] */
	s64 = (-0x7fffffffffffffffLL - 1LL) % -1LL;
	s64 = 0x7fffffffffffffffLL % -1LL;
}

void
mod_u64(void)
{
	u64 = 0xffffffffffffffffULL % -1ULL;
	u64 = 0xffffffffffffffffULL % 1ULL;
	/* expect+1: error: modulus by 0 [140] */
	u64 = 0ULL % 0ULL;
	/* expect+1: error: modulus by 0 [140] */
	u64 = 0xffffffffffffffffULL % 0ULL;
}

void
plus_s32(void)
{
	/* expect+1: warning: operator '+' produces integer overflow [141] */
	s32 = -0x7fffffff + -0x7fffffff;
	/* expect+1: warning: operator '+' produces integer overflow [141] */
	s32 = -0x7fffffff + -2;			// INT_MIN - 1
	/* expect+1: warning: operator '+' produces integer overflow [141] */
	s32 = -2 + -0x7fffffff;			// INT_MIN - 1
	s32 = -0x7fffffff + -1;			// INT_MIN
	s32 = -1 + -0x7fffffff;			// INT_MIN
	s32 = -0x7fffffff + 0;			// INT_MIN + 1
	s32 = 0 + -0x7fffffff;			// INT_MIN + 1
	s32 = (-0x7fffffff - 1) + 0x7fffffff;	// -1
	s32 = 0x7fffffff + (-0x7fffffff - 1);	// -1
	s32 = 0x7ffffffe + 1;			// INT_MAX
	s32 = 1 + 0x7ffffffe;			// INT_MAX
	s32 = 0x7fffffff + 0;			// INT_MAX
	s32 = 0 + 0x7fffffff;			// INT_MAX
	/* expect+1: warning: operator '+' produces integer overflow [141] */
	s32 = 0x7fffffff + 1;			// INT_MAX + 1
	/* expect+1: warning: operator '+' produces integer overflow [141] */
	s32 = 1 + 0x7fffffff;			// INT_MAX + 1
	/* expect+1: warning: operator '+' produces integer overflow [141] */
	s32 = 0x40000000 + 0x40000000;		// INT_MAX + 1
	/* expect+1: warning: operator '+' produces integer overflow [141] */
	s32 = 0x7fffffff + 0x7fffffff;
}

void
plus_u32(void)
{
	u32 = 0x00000000U + 0x00000000U;
	u32 = 0x40000000U + 0x40000000U;
	u32 = 0xffffffffU + 0x00000000U;
	u32 = 0x00000000U + 0xffffffffU;
	u32 = 0xfffffffeU + 0x00000001U;
	/* expect+1: warning: operator '+' produces integer overflow [141] */
	u32 = 0xffffffffU + 0x00000001U;
	/* expect+1: warning: operator '+' produces integer overflow [141] */
	u32 = 0x00000001U + 0xffffffffU;
	/* expect+1: warning: operator '+' produces integer overflow [141] */
	u32 = 0xffffffffU + 0xffffffffU;
}

void
plus_s64(void)
{
	/* expect+1: warning: operator '+' produces integer overflow [141] */
	s64 = -0x7fffffffffffffffLL + -2LL;
	s64 = -0x7fffffffffffffffLL + -1LL;
	s64 = 0x7ffffffffffffffeLL + 1LL;
	/* expect+1: warning: operator '+' produces integer overflow [141] */
	s64 = 0x7fffffffffffffffLL + 1LL;
}

void
plus_u64(void)
{
	u64 = 0x0000000000000000ULL + 0x0000000000000000ULL;
	u64 = 0xffffffffffffffffULL + 0x0000000000000000ULL;
	u64 = 0x0000000000000000ULL + 0xffffffffffffffffULL;
	u64 = 0xfffffffffffffffeULL + 0x0000000000000001ULL;
	/* TODO: expect+1: warning: operator '+' produces integer overflow [141] */
	u64 = 0xffffffffffffffffULL + 0x0000000000000001ULL;
	/* TODO: expect+1: warning: operator '+' produces integer overflow [141] */
	u64 = 0x0000000000000001ULL + 0xffffffffffffffffULL;
	/* TODO: expect+1: warning: operator '+' produces integer overflow [141] */
	u64 = 0xffffffffffffffffULL + 0xffffffffffffffffULL;
}

void
minus_s32(void)
{
	/* expect+1: warning: operator '-' produces integer overflow [141] */
	s32 = -0x7fffffff - 2;
	s32 = -0x7fffffff - 1;
	s32 = -0x7fffffff - 1 - 0;
	s32 = -0x7fffffff - 1 - -1;
	s32 = 0x7fffffff - 0x7fffffff;
	s32 = 0x7fffffff - 1;
	s32 = 0x7fffffff - 0;
	/* expect+1: warning: operator '-' produces integer overflow [141] */
	s32 = 0x7fffffff - -1;
}

void
minus_u32(void)
{
	u32 = 0x00000000U - 0x00000000U;
	/* TODO: expect+1: warning: operator '-' produces integer overflow [141] */
	u32 = 0x00000000U - 0x00000001U;
	/* expect+1: warning: operator '-' produces integer overflow [141] */
	u32 = 0x00000000U - 0x80000000U;
	u32 = 0x80000000U - 0x00000001U;
	/* TODO: expect+1: warning: operator '-' produces integer overflow [141] */
	u32 = 0x00000000U - 0xffffffffU;
	u32 = 0xffffffffU - 0x00000000U;
	u32 = 0xffffffffU - 0xffffffffU;
}

void
minus_s64(void)
{
	/* expect+1: warning: operator '-' produces integer overflow [141] */
	s64 = -0x7fffffffffffffffLL - 0x7fffffffffffffffLL;
	/* expect+1: warning: operator '-' produces integer overflow [141] */
	s64 = -0x7fffffffffffffffLL - 2LL;
	s64 = -0x7fffffffffffffffLL - 1LL;
	s64 = -0x7fffffffffffffffLL - 0LL;
	s64 = -0x7fffffffffffffffLL - -1LL;
	s64 = 0x7fffffffffffffffLL - 1LL;
	s64 = 0x7fffffffffffffffLL - 0LL;
	/* expect+1: warning: operator '-' produces integer overflow [141] */
	s64 = 0x7fffffffffffffffLL - -1LL;
	/* expect+1: warning: operator '-' produces integer overflow [141] */
	s64 = 0x7fffffffffffffffLL - -0x7fffffffffffffffLL;
}

void
minus_u64(void)
{
	// TODO
}

void
shl_s32(void)
{
	/* TODO: expect+1: warning: operator '<<' produces integer overflow [141] */
	s32 = 0x0100 << 23;
	/* expect+1: warning: operator '<<' produces integer overflow [141] */
	s32 = 0x0100 << 24;
	/* expect+1: warning: shift amount 18446744073709551615 is greater than bit-size 32 of 'int' [122] */
	s32 = 0 << 0xffffffffffffffff;
}

void
shl_u32(void)
{
	u32 = 0x0100U << 23;
	/* expect+1: warning: operator '<<' produces integer overflow [141] */
	u32 = 0x0100U << 24;
	/* expect+1: warning: negative shift [121] */
	u32 = 0x0000U << -1;
	/* expect+1: warning: shift amount 256 is greater than bit-size 32 of 'unsigned int' [122] */
	u32 = 0x0100U << 0x0100U;
	/* expect+1: warning: shift amount 4294967295 is greater than bit-size 32 of 'unsigned int' [122] */
	u32 = 0x0100U << 0xffffffffU;
	/* expect+1: warning: shift amount 18446744073709551615 is greater than bit-size 32 of 'unsigned int' [122] */
	u32 = 0x0100U << 0xffffffffffffffffULL;
}

void
shl_s64(void)
{
	// TODO
}

void
shl_u64(void)
{
	// TODO
}

void
shr_s32(void)
{
	s32 = -0x7fffffff >> 1;
	s32 = -10 >> 1;
	s32 = -9 >> 1;
	s32 = +9 >> 1;
	s32 = +10 >> 1;
	s32 = 0x7fffffff >> 1;
}

void
shr_u32(void)
{
	u32 = 0xffffffffU >> 1;
	/* expect+1: warning: shift amount 32 equals bit-size of 'unsigned int' [267] */
	u32 = 0xffffffffU >> 32;
	u32 = 0x00000000U >> 1;
}

void
shr_s64(void)
{
	// TODO
}

void
shr_u64(void)
{
	// TODO
}

void
compare_s32(void)
{
	// TODO
}

void
compare_u32(void)
{
	// TODO
}

void
compare_s64(void)
{
	// TODO
}

void
compare_u64(void)
{
	// TODO
}

void
bitand_s32(void)
{
	s32 = 0x55555555 & -0xff;
}

void
bitand_u32(void)
{
	u32 = 0xffffffffU & 0x55555555U;
	u32 = 0x55555555U & 0xaaaaaaaaU;
}

void
bitand_s64(void)
{
	// TODO
}

void
bitand_u64(void)
{
	// TODO
}

void
bitxor_s32(void)
{
	s32 = 0x12345678 ^ 0x76543210;
}

void
bitxor_u32(void)
{
	u32 = 0xffffffffU ^ 0x55555555U;
	u32 = 0x55555555U ^ 0xaaaaaaaaU;
}

void
bitxor_s64(void)
{
	// TODO
}

void
bitxor_u64(void)
{
	// TODO
}

void
bitor_s32(void)
{
	s32 = 0x3333cccc | 0x5555aaaa;
}

void
bitor_u32(void)
{
	u32 = 0xffffffffU | 0x00000000U;
	u32 = 0xffffffffU | 0xffffffffU;
	u32 = 0x55555555U | 0xaaaaaaaaU;
}

void
bitor_s64(void)
{
	// TODO
}

void
bitor_u64(void)
{
	// TODO
}
