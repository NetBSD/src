/*	$NetBSD: platform_schar.c,v 1.6 2024/06/08 06:37:06 rillig Exp $	*/
# 3 "platform_schar.c"

/*
 * Test features that only apply to platforms where plain char has the same
 * representation as signed char.
 */

/* lint1-extra-flags: -c -h -a -p -b -r -z -X 161,351 */
/* lint1-only-if: schar */

/* expect+1: warning: nonportable character comparison '-128 < ?' [230] */
typedef int char_char[(char)'\200' < (char)'\177' ? 1 : -1];
/* expect+1: warning: nonportable character comparison '-128 < ?' [230] */
typedef int char_int[(char)'\200' < (char)127 ? 1 : -1];
/* expect+1: warning: nonportable character comparison '-128 < ?' [230] */
typedef int int_char[(char)-128 < (char)'\177' ? 1 : -1];
/* expect+1: warning: nonportable character comparison '-128 < ?' [230] */
typedef int int_int[(char)-128 < (char)127 ? 1 : -1];


void
first_to_upper(char *p)
{
	*p += 'A' - 'a';
}
