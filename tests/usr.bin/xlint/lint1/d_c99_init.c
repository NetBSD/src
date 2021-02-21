/*	$NetBSD: d_c99_init.c,v 1.1 2021/02/21 08:05:51 rillig Exp $	*/
# 3 "d_c99_init.c"

/*
 * Test C99 initializers.
 *
 * See C99 6.7.8 "Initialization".
*/

/* lint1-extra-flags: -p */

// C99 6.7.8p11 says "optionally enclosed in braces".  The intended
// interpretation is "optionally enclosed in a single pair of braces".
int scalar_without_braces = 3;
int scalar_with_optional_braces = { 3 };
int scalar_with_too_many_braces = {{ 3 }};
