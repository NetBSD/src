/*	$NetBSD: platform_schar.c,v 1.5 2023/12/02 23:54:49 rillig Exp $	*/
# 3 "platform_schar.c"

/*
 * Test features that only apply to platforms where plain char has the same
 * representation as signed char.
 */

/* lint1-extra-flags: -c -h -a -p -b -r -z -X 161 */
/* lint1-only-if: schar */

/* expect+1: warning: nonportable character comparison '-128 < ?' [230] */
typedef int char_char[(char)'\200' < (char)'\177' ? 1 : -1];
/* expect+1: warning: nonportable character comparison '-128 < ?' [230] */
typedef int char_int[(char)'\200' < (char)127 ? 1 : -1];
/* expect+1: warning: nonportable character comparison '-128 < ?' [230] */
typedef int int_char[(char)-128 < (char)'\177' ? 1 : -1];
/* expect+1: warning: nonportable character comparison '-128 < ?' [230] */
typedef int int_int[(char)-128 < (char)127 ? 1 : -1];
