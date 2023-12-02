/*	$NetBSD: platform_uchar.c,v 1.4 2023/12/02 23:54:49 rillig Exp $	*/
# 3 "platform_uchar.c"

/*
 * Test features that only apply to platforms where plain char has the same
 * representation as unsigned char.
 */

/* lint1-extra-flags: -c -h -a -p -b -r -z -X 161 */
/* lint1-only-if: uchar */

/* expect+1: warning: nonportable character comparison '< 128' [230] */
typedef int char_char[(char)'\177' < (char)'\200' ? 1 : -1];
/* expect+1: warning: nonportable character comparison '< 128' [230] */
typedef int int_char[(char)127 < (char)'\200' ? 1 : -1];
/* expect+1: warning: nonportable character comparison '< 128' [230] */
typedef int char_int[(char)'\177' < (char)128 ? 1 : -1];
/* expect+1: warning: nonportable character comparison '< 128' [230] */
typedef int int_int[(char)127 < (char)128 ? 1 : -1];
