/*	$NetBSD: expr_fold.c,v 1.6 2022/04/15 21:50:07 rillig Exp $	*/
# 3 "expr_fold.c"

/*
 * Test folding of constant expressions.
 */

/* lint1-extra-flags: -h */

/*
 * On ILP32 platforms, the integer constant 2147483648 cannot be represented
 * as 'int' or 'long', therefore it becomes 'long long'.  This would
 * influence the type names in the diagnostics.
 */
/* lint1-only-if: lp64 */

void take_bool(_Bool);
void take_int(int);
void take_uint(unsigned int);

/*
 * C99 6.4.4.1p5 defines that decimal integer constants without suffix get
 * one of the signed integer types. On the other hand, octal and hexadecimal
 * constants get either a signed or unsigned type, whichever fits first.
 */

void
fold_uplus(void)
{
	take_int(+(0));
	take_int(+(2147483647));
	/* expect+1: warning: conversion of 'long' to 'int' is out of range, arg #1 [295] */
	take_int(+(2147483648));
	/* expect+1: warning: conversion of 'long' to 'int' is out of range, arg #1 [295] */
	take_int(+(4294967295));

	take_uint(+(0));
	take_uint(+(2147483647));
	take_uint(+(2147483648));
	take_uint(+(4294967295));

	/*
	 * Hexadecimal constants and constants with the suffix 'U' get either
	 * a signed or an unsigned integer type, so no warning here.
	 */
	take_uint(+(2147483648U));
	take_uint(+(0x80000000));
	take_uint(+(0x80000000U));
}

void
fold_uminus(void)
{
	take_int(-(0));
	take_int(-(2147483647));

	take_int(-(2147483648));

	/* The '-' is an operator, it is not part of the integer constant. */
	take_int(-2147483648);

	/* expect+2: warning: integer overflow detected, op '+' [141] */
	/* expect+1: warning: integer overflow detected, op '-' [141] */
	take_int(-(2147483647 + 1));
	/* expect+1: warning: integer overflow detected, op '-' [141] */
	take_int(-(-2147483647 - 1));
	/* expect+1: warning: conversion of 'long' to 'int' is out of range, arg #1 [295] */
	take_int(-(4294967295));

	take_uint(-(0));
	/* expect+1: warning: conversion of negative constant to unsigned type, arg #1 [296] */
	take_uint(-(2147483647));
	/* expect+1: warning: conversion of negative constant to unsigned type, arg #1 [296] */
	take_uint(-(2147483648));
	/* expect+1: warning: conversion of negative constant to unsigned type, arg #1 [296] */
	take_uint(-(4294967295));
}

void
fold_compl(void)
{
	take_int(~(0));
	take_int(~(2147483647));
	/* expect+1: warning: conversion of 'long' to 'int' is out of range, arg #1 [295] */
	take_int(~(2147483648));
	/* expect+1: warning: conversion of 'long' to 'int' is out of range, arg #1 [295] */
	take_int(~(4294967295));

	/* expect+1: warning: conversion of negative constant to unsigned type, arg #1 [296] */
	take_uint(~(0));
	/* expect+1: warning: conversion of negative constant to unsigned type, arg #1 [296] */
	take_uint(~(2147483647));
	/* expect+1: warning: conversion of negative constant to unsigned type, arg #1 [296] */
	take_uint(~(2147483648));
	/* expect+1: warning: conversion of negative constant to unsigned type, arg #1 [296] */
	take_uint(~(4294967295));
}

void
fold_mult(void)
{
	take_int(32767 * 65536);
	/* expect+1: warning: integer overflow detected, op '*' [141] */
	take_int(32768 * 65536);
	/* expect+1: warning: integer overflow detected, op '*' [141] */
	take_int(65536 * 65536);

	take_uint(32767 * 65536U);
	take_uint(32768 * 65536U);
	/* expect+1: warning: integer overflow detected, op '*' [141] */
	take_uint(65536 * 65536U);
}

void
fold_div(void)
{
	/* expect+3: error: division by 0 [139] */
	/* XXX: The following message is redundant. */
	/* expect+1: warning: integer overflow detected, op '/' [141] */
	take_int(0 / 0);

	/* expect+1: warning: conversion of 'long' to 'int' is out of range, arg #1 [295] */
	take_int(-2147483648 / -1);
}

void
fold_mod(void)
{
	/* expect+1: error: modulus by 0 [140] */
	take_int(0 % 0);
	/* expect+1: error: modulus by 0 [140] */
	take_int(0 % 0U);
	/* expect+1: error: modulus by 0 [140] */
	take_int(0U % 0);
	/* expect+1: error: modulus by 0 [140] */
	take_int(0U % 0U);

	take_int(-2147483648 % -1);
}

void
fold_plus(void)
{
	/* expect+1: warning: integer overflow detected, op '+' [141] */
	take_int(2147483647 + 1);

	/* Assume two's complement, so no overflow. */
	take_int(-2147483647 + -1);

	/* expect+1: warning: integer overflow detected, op '+' [141] */
	take_int(-2147483647 + -2);

	/*
	 * No overflow since one of the operands is unsigned, therefore the
	 * other operand is converted to unsigned as well.
	 * See C99 6.3.1.8p1, paragraph 8 of 10.
	 */
	/* wrong integer overflow warning before tree.c 1.338 from 2021-08-19 */
	take_uint(2147483647 + 1U);
	/* wrong integer overflow warning before tree.c 1.338 from 2021-08-19 */
	take_uint(2147483647U + 1);
}

void
fold_minus(void)
{
	/* expect+1: warning: integer overflow detected, op '-' [141] */
	take_int(2147483647 - -1);
	/* Assume two's complement. */
	take_int(-2147483647 - 1);
	/* expect+1: warning: integer overflow detected, op '-' [141] */
	take_int(-2147483647 - 2);

	take_int(0 - 2147483648);
	/* expect+1: warning: integer overflow detected, op '-' [141] */
	take_uint(0 - 2147483648U);
}

void
fold_shl(void)
{
	/* expect+1: warning: integer overflow detected, op '<<' [141] */
	take_int(1 << 24 << 24);

	/* expect+1: warning: integer overflow detected, op '<<' [141] */
	take_uint(1U << 24 << 24);

	/* FIXME: undefined behavior in 'fold' at 'uint64_t << 104'. */
	/* expect+1: warning: shift amount 104 is greater than bit-size 32 of 'unsigned int' [122] */
	take_uint(1U << 24 << 104);
}

void
fold_shr(void)
{
	take_int(16777216 >> 24);

	take_int(16777216 >> 25);

	/* FIXME: undefined behavior in 'fold' at 'uint64_t >> 104'. */
	/* expect+1: warning: shift amount 104 is greater than bit-size 32 of 'int' [122] */
	take_int(16777216 >> 104);
}

void
fold_lt(void)
{
	take_bool(1 < 3);
	take_bool(3 < 1);
}

void
fold_le(void)
{
	take_bool(1 <= 3);
	take_bool(3 <= 1);
}

void
fold_ge(void)
{
	take_bool(1 >= 3);
	take_bool(3 >= 1);
}

void
fold_gt(void)
{
	take_bool(1 > 3);
	take_bool(3 > 1);
}

void
fold_eq(void)
{
	take_bool(1 == 3);
	take_bool(3 == 1);
}

void
fold_ne(void)
{
	take_bool(1 != 3);
	take_bool(3 != 1);
}

void
fold_bitand(void)
{
	take_bool(1 & 3);
	take_bool(3 & 1);
}

void
fold_bitxor(void)
{
	take_bool(1 ^ 3);
	take_bool(3 ^ 1);
}

void
fold_bitor(void)
{
	take_bool(1 | 3);
	take_bool(3 | 1);
}

/*
 * The following expression originated in vndcompress.c 1.29 from 2017-07-29,
 * where line 310 contained a seemingly harmless compile-time assertion that
 * expanded to a real monster expression.
 *
 * __CTASSERT(MUL_OK(uint64_t, MAX_N_BLOCKS, MAX_BLOCKSIZE));
 *
 * Before tree.c 1.345 from 2021-08-22, lint wrongly assumed that the result
 * of all binary operators were the common arithmetic type, but that was
 * wrong for the comparison operators.  The expression '1ULL < 2ULL' does not
 * have type 'unsigned long long' but 'int' in default mode, or '_Bool' in
 * strict bool mode.
 */
struct ctassert5_struct {
	unsigned int member:
	    /*CONSTCOND*/
	    0xfffffffeU
	    <=
		((1ULL << 63) + 1 < 1 ? ~(1ULL << 63) : ~0ULL) / 0xfffffe00U
		? 1
		: -1;
};
