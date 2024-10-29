/*	$NetBSD: msg_056.c,v 1.5 2024/10/29 20:48:31 rillig Exp $	*/
# 3 "msg_056.c"

// Test for message: constant %s too large for 'int' [56]

/* lint1-extra-flags: -h */

enum {
	S31_MAX = 0x7FFFFFFF,
	U31_MAX = 0x7FFFFFFFU,

	// The hexadecimal constant has type 'int', since it fits.
	/* expect+1: warning: '2147483647 + 1' overflows 'int' [141] */
	S31_MAX_PLUS_1 = 0x7FFFFFFF + 1,

	/* expect+1: warning: constant 0x80000000 too large for 'int' [56] */
	U31_MAX_PLUS_1 = 0x7FFFFFFFU + 1,


	/* expect+1: warning: constant 0xffffffff too large for 'int' [56] */
	U32_MAX = 0xFFFFFFFF,

	/* expect+2: warning: '9223372036854775807 + 1' overflows 'long' [141] */
	/* expect+1: warning: constant 0x7fffffffffffffff too large for 'int' [56] */
	S63_MAX_PLUS_1 = 0x7FFFFFFFFFFFFFFF + 1,

	/* expect+1: warning: constant -0x8000000000000000 too large for 'int' [56] */
	S63_MIN = -0x7FFFFFFFFFFFFFFF - 1,

	/* expect+1: warning: constant 0x7fffffffffffffff too large for 'int' [56] */
	U63_MAX = 0x7FFFFFFFFFFFFFFF,

	/* expect+1: warning: constant 0xffffffffffffffff too large for 'int' [56] */
	U64_MAX = 0xFFFFFFFFFFFFFFFF,

	/* expect+2: warning: integer constant out of range [252] */
	/* expect+1: warning: constant 0xffffffffffffffff too large for 'int' [56] */
	U80_MAX = 0xFFFFFFFFFFFFFFFFFFFF,
};
