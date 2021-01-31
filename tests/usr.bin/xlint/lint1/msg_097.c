/*	$NetBSD: msg_097.c,v 1.3 2021/01/31 11:12:07 rillig Exp $	*/
# 3 "msg_097.c"

/* Test for message: suffix U is illegal in traditional C [97] */

/* lint1-flags: -gtw */

void
example()
{
	int i = 1234567;
	unsigned u = 1234567;
	unsigned u_upper = 1234567U;		/* expect: 97 */
	unsigned u_lower = 1234567u;		/* expect: 97 */

	long l = 1234567L;
	unsigned long ul = 1234567UL;		/* expect: 97 */

	long long ll = 1234567LL;
	unsigned long long ull = 1234567ULL;	/* expect: 97 */
}
