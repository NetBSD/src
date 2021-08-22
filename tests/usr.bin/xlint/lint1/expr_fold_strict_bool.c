/*	$NetBSD: expr_fold_strict_bool.c,v 1.2 2021/08/22 21:17:04 rillig Exp $	*/
# 3 "expr_fold_strict_bool.c"

/*
 * Test constant folding in strict bool mode.
 *
 * In this mode, _Bool is not an unsigned integer type.  In fact, it is not
 * an arithmetic type at all.
 */

/* lint1-extra-flags: -T */
/* lint1-only-if: lp64 */

typedef long long int64_t;
typedef unsigned long long uint64_t;

struct fold_64_bit {

	_Bool lt_signed_small_ok: -3LL < 1LL ? 1 : -1;
	/* expect+1: error: illegal bit-field size: 255 [36] */
	_Bool lt_signed_small_bad: 1LL < -3LL ? 1 : -1;

	_Bool lt_signed_big_ok: (int64_t)(1ULL << 63) < 1LL ? 1 : -1;
	/* expect+1: error: illegal bit-field size: 255 [36] */
	_Bool lt_signed_big_bad: 1LL < (int64_t)(1ULL << 63) ? 1 : -1;

	_Bool lt_unsigned_small_ok: 1ULL < 3ULL ? 1 : -1;
	/* expect+1: error: illegal bit-field size: 255 [36] */
	_Bool lt_unsigned_small_bad: 3ULL < 1ULL ? 1 : -1;

	/*
	 * Before tree.c 1.345 from 2021-08-22, lint wrongly assumed that the
	 * result of all binary operators were the common arithmetic type,
	 * but that was wrong for the comparison operators.  The expression
	 * '1ULL < 2ULL' does not have type 'unsigned long long' but 'int' in
	 * default mode, or '_Bool' in strict bool mode.
	 */
	_Bool lt_unsigned_big_ok: 1ULL < 1ULL << 63 ? 1 : -1;
	/* expect+1: error: illegal bit-field size: 255 [36] */
	_Bool lt_unsigned_big_bad: 1ULL << 63 < 1ULL ? 1 : -1;
};
