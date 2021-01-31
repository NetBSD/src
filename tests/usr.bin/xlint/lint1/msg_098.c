/*	$NetBSD: msg_098.c,v 1.3 2021/01/31 11:12:07 rillig Exp $	*/
# 3 "msg_098.c"

/* Test for message: suffixes F and L are illegal in traditional C [98] */

/* lint1-flags: -gtw */

void
example()
{
	float f = 1234.5;
	float f_F = 1234.5F;		/* expect: 98 */
	float f_f = 1234.5f;		/* expect: 98 */

	double d = 1234.5;
	double d_U = 1234.5U;		/* expect: 249 */

	long double ld = 1234.5;	/* expect: 266 */
	long double ld_L = 1234.5L;	/* expect: 98, 266 */
}
