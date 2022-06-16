/*	$NetBSD: msg_166.c,v 1.4 2022/06/16 16:58:36 rillig Exp $	*/
# 3 "msg_166.c"

// Test for message: precision lost in bit-field assignment [166]

/* lint1-extra-flags: -hp */

struct bit_set {

	/*
	 * C99 6.7.2p5 and 6.7.2.1p9 footnote 104 say that for bit-fields of
	 * underlying type 'int', "it is implementation-defined whether the
	 * specifier 'int' designates the same type as 'signed int' or the
	 * same type as 'unsigned int'".
	 *
	 * https://gcc.gnu.org/onlinedocs/gcc/Structures-unions-enumerations
	 * -and-bit-fields-implementation.html says: "By default it is treated
	 * as 'signed int' but this may be changed by the
	 * '-funsigned-bitfields' option".
	 *
	 * Clang doesn't document implementation-defined behavior, see
	 * https://bugs.llvm.org/show_bug.cgi?id=11272.
	 */

	/* expect+1: warning: bit-field of type plain 'int' has implementation-defined signedness [344] */
	int minus_1_to_0: 1;
	/* expect+1: warning: bit-field of type plain 'int' has implementation-defined signedness [344] */
	int minus_8_to_7: 4;
	unsigned zero_to_1: 1;
	unsigned zero_to_15: 4;
};

void example(void) {
	struct bit_set bits;

	/* Clang doesn't warn about the 1. */
	/* expect+1: warning: precision lost in bit-field assignment [166] */
	bits.minus_1_to_0 = -2;
	bits.minus_1_to_0 = -1;
	bits.minus_1_to_0 = 0;
	/* expect+1: warning: precision lost in bit-field assignment [166] */
	bits.minus_1_to_0 = 1;
	/* expect+1: warning: precision lost in bit-field assignment [166] */
	bits.minus_1_to_0 = 2;

	/* expect+1: warning: precision lost in bit-field assignment [166] */
	bits.minus_8_to_7 = -9;
	bits.minus_8_to_7 = -8;
	bits.minus_8_to_7 = 7;
	/* expect+1: warning: precision lost in bit-field assignment [166] */
	bits.minus_8_to_7 = 8;

	/* Clang doesn't warn about the -1. */
	/* expect+1: warning: assignment of negative constant to unsigned type [164] */
	bits.zero_to_1 = -2;
	/* expect+1: warning: assignment of negative constant to unsigned type [164] */
	bits.zero_to_1 = -1;
	bits.zero_to_1 = 0;
	bits.zero_to_1 = 1;
	/* expect+1: warning: precision lost in bit-field assignment [166] */
	bits.zero_to_1 = 2;

	/* Clang doesn't warn about the -8. */
	/* expect+1: warning: assignment of negative constant to unsigned type [164] */
	bits.zero_to_15 = -9;
	/* expect+1: warning: assignment of negative constant to unsigned type [164] */
	bits.zero_to_15 = -8;
	bits.zero_to_15 = 0;
	bits.zero_to_15 = 15;
	/* expect+1: warning: precision lost in bit-field assignment [166] */
	bits.zero_to_15 = 16;
}
