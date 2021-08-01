/*	$NetBSD: expr_binary.c,v 1.3 2021/08/01 13:49:17 rillig Exp $	*/
# 3 "expr_binary.c"

/*
 * Test binary operators, in particular the usual arithmetic conversions.
 *
 * C99 6.3.1.8 "Usual arithmetic conversions"
 */

/* lint1-extra-flags: -Ac11 */
/* lint1-only-if: lp64 */

struct incompatible {		/* just to generate the error message */
	int member;
};
void sink(struct incompatible);

void
cover_balance(void)
{
	/* expect+1: 'int' */
	sink(1 + '\0');

	/* expect+1: 'int' */
	sink((char)'\0' + (char)'\0');

	/* expect+1: 'float' */
	sink(1 + 1.0f);

	/* expect+1: 'double' */
	sink(1 + 1.0);

	/* expect+1: 'float' */
	sink((long long)1 + 1.0f);

	/* expect+1: 'float' */
	sink((long long)1 + 1.0f);

	/* expect+1: 'float' */
	sink((__uint128_t)1 + 1.0f);

	/* expect+1: '__uint128_t' */
	sink((__uint128_t)1 + 1);

	/* expect+1: '__int128_t' */
	sink((__int128_t)1 + 1);

	/* expect+1: '__uint128_t' */
	sink((__uint128_t)1 + (__int128_t)1);

	/* expect+1: '__uint128_t' */
	sink((__int128_t)1 + (__uint128_t)1);
}
