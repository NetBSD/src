/*	$NetBSD: platform_ilp32.c,v 1.4 2023/02/27 23:07:53 rillig Exp $	*/
# 3 "platform_ilp32.c"

/*
 * Test features that only apply to platforms that have 32-bit int, long and
 * pointer types.
 *
 * See also:
 *	platform_ilp32_int.c
 *	platform_ilp32_long.c
 */

/* lint1-extra-flags: -c -h -a -p -b -r -z */
/* lint1-only-if: ilp32 */

typedef int do_not_warn_about_empty_translation_unit;
