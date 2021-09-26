/*	$NetBSD: platform_schar.c,v 1.2 2021/09/26 14:28:22 rillig Exp $	*/
# 3 "platform_schar.c"

/*
 * Test features that only apply to platforms where plain char has the same
 * representation as signed char.
 */

/* lint1-extra-flags: -c -h -a -p -b -r -z */
/* lint1-only-if: schar */

// TODO: Add some code that passes.
// TODO: Add some code that fails.

/* expect+1: warning: empty translation unit [272] */
