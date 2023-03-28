/*	$NetBSD: emit_lp64.c,v 1.2 2023/03/28 14:44:34 rillig Exp $	*/
# 3 "emit_lp64.c"

/*
 * Test the symbol information that lint1 writes to a .ln file.  Using this
 * symbol information, lint2 later checks that the symbols are used
 * consistently across different translation units.
 *
 * This test covers large integer types that are only supported on LP64
 * platforms.
 */

// omit the option '-g' to avoid having the GCC builtins in the .ln file.
/* lint1-flags: -Sw -X 351 */

/* lint1-only-if: lp64 */

__int128_t int128(__int128_t);
__uint128_t uint128(__uint128_t);
