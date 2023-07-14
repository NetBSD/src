/*	$NetBSD: expr_binary.c,v 1.8 2023/07/14 08:53:52 rillig Exp $	*/
# 3 "expr_binary.c"

/*
 * Test binary operators.
 */

/* lint1-only-if: lp64 */
/* lint1-extra-flags: -X 351 */

struct incompatible {		/* just to generate the error message */
	int member;
};
void sink(struct incompatible);

/*
 * Test the usual arithmetic conversions.
 *
 * C99 6.3.1.8 "Usual arithmetic conversions"
 */
void
cover_balance(void)
{
	/* expect+1: ... 'pointer to void' ... */
	sink((void *)0 + 0);

	/* expect+1: ... 'pointer to void' ... */
	sink(0 + (void *)0);

	/* expect+1: ... 'int' ... */
	sink(1 + 1);

	/* expect+1: ... 'const int' ... */
	sink((const int)1 + (volatile int)1);

	/* expect+1: ... 'volatile int' ... */
	sink((volatile int)1 + (const int)1);

	long double _Complex cldbl = 0.0;
	double _Complex cdbl = 0.0;
	float _Complex cflt = 0.0f;
	/* expect+1: error: invalid type for _Complex [308] */
	_Complex invalid = 0.0;

	/* expect+1: ... 'long double _Complex' ... */
	sink(cldbl + 0);
	/* expect+1: ... 'long double _Complex' ... */
	sink(0 + cldbl);
	/* expect+1: ... 'long double _Complex' ... */
	sink(cldbl + cdbl);
	/* expect+1: ... 'long double _Complex' ... */
	sink(cdbl + cldbl);

	/* expect+1: ... 'double _Complex' ... */
	sink(cdbl + 0);
	/* expect+1: ... 'double _Complex' ... */
	sink(0 + cdbl);
	/* expect+1: ... 'double _Complex' ... */
	sink(cdbl + cflt);
	/* expect+1: ... 'double _Complex' ... */
	sink(cflt + cdbl);

	/* expect+1: ... 'float _Complex' ... */
	sink(cflt + 0);
	/* expect+1: ... 'float _Complex' ... */
	sink(0 + cflt);
	/* expect+1: ... 'float _Complex' ... */
	sink(cflt + (__uint128_t)0);
	/* expect+1: ... 'float _Complex' ... */
	sink((__uint128_t)0 + cflt);

	/*
	 * The type specifier '_Complex' is only used during parsing, it does
	 * not make it to the expression.
	 */
	/* expect+1: ... 'double _Complex' ... */
	sink(invalid + 0);

	/* expect+1: ... 'long double' ... */
	sink(0.0L + 0);
	/* expect+1: ... 'long double' ... */
	sink(0 + 0.0L);
	/* expect+1: ... 'long double' ... */
	sink(0.0L + 0.0);
	/* expect+1: ... 'long double' ... */
	sink(0.0 + 0.0L);

	/* expect+1: ... 'double' ... */
	sink(0.0 + 0);
	/* expect+1: ... 'double' ... */
	sink(0 + 0.0);
	/* expect+1: ... 'double' ... */
	sink(0.0 + 0.0f);
	/* expect+1: ... 'double' ... */
	sink(0.0f + 0.0);

	/* expect+1: ... 'float' ... */
	sink(0.0f + 0);
	/* expect+1: ... 'float' ... */
	sink(0 + 0.0f);
	/* expect+1: ... 'float' ... */
	sink(0.0f + (__uint128_t)0);
	/* expect+1: ... 'float' ... */
	sink((__uint128_t)0 + 0.0f);

	/* expect+1: ... 'unsigned long long' ... */
	sink(0ULL + 0);
	/* expect+1: ... 'unsigned long long' ... */
	sink(0 + 0ULL);

	/* expect+1: ... 'unsigned long long' ... */
	sink(0ULL + 0LL);
	/* expect+1: ... 'unsigned long long' ... */
	sink(0LL + 0ULL);

	/* If the bit-width is the same, prefer the unsigned variant. */
	/* expect+1: ... 'unsigned long long' ... */
	sink(0UL + 0LL);
	/* expect+1: ... 'unsigned long long' ... */
	sink(0LL + 0UL);

	/*
	 * Ensure that __int128_t is listed in the integer ranks.  This table
	 * only becomes relevant when both operands have the same width.
	 */
	/* expect+1: ... '__uint128_t' ... */
	sink((__uint128_t)1 + (__int128_t)1);
	/* expect+1: ... '__uint128_t' ... */
	sink((__int128_t)1 + (__uint128_t)1);
}

struct point {
	int x, y;
};

static struct point
returning_struct(void)
{
	return (struct point){ 0, 0 };
}

static void
returning_void(void)
{
}

static inline void
op_colon(_Bool cond)
{
	// FIXME: GCC doesn't warn, as the 'type mismatch' is not wrong.
	/* expect+1: warning: incompatible types 'struct point' and 'void' in conditional [126] */
	cond ? returning_struct() : returning_void();

	// TODO: Test the other combinations as well.
	// |         | void | bool | arith | sou | int | flt | ptr | nullptr |
	// |---------|------|------|-------|-----|-----|-----|-----|---------|
	// | void    | ok   |      |       |     |     |     |     |         |
	// | bool    |      | ok   |       |     |     |     |     |         |
	// | arith   |      |      | ok    |     |     |     |     |         |
	// | sou     |      |      |       | ok  |     |     |     |         |
	// | int     |      |      |       |     |     |     | ok  |         |
	// | flt     |      |      |       |     |     |     |     |         |
	// | ptr     |      |      |       |     | ok  |     |     | ok      |
	// | nullptr |      |      |       |     |     |     | ok  |         |
}
