/*	$NetBSD: platform_uchar.c,v 1.3 2023/02/22 22:12:35 rillig Exp $	*/
# 3 "platform_uchar.c"

/*
 * Test features that only apply to platforms where plain char has the same
 * representation as unsigned char.
 */

/* lint1-extra-flags: -c -h -a -p -b -r -z */
/* lint1-only-if: uchar */

/* CONSTCOND */
/* expect+1: warning: nonportable character comparison '< 128' [230] */
typedef int is_unsigned[(char)'\177' < (char)'\200' ? 1 : -1];
