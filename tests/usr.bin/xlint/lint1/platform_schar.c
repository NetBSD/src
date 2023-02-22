/*	$NetBSD: platform_schar.c,v 1.4 2023/02/22 22:30:40 rillig Exp $	*/
# 3 "platform_schar.c"

/*
 * Test features that only apply to platforms where plain char has the same
 * representation as signed char.
 */

/* lint1-extra-flags: -c -h -a -p -b -r -z */
/* lint1-only-if: schar */

/* CONSTCOND */
/* expect+1: warning: nonportable character comparison '-128 < ?' [230] */
typedef int is_signed[(char)'\200' < (char)'\177' ? 1 : -1];
