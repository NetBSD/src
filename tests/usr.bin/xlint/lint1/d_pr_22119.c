/*	$NetBSD: d_pr_22119.c,v 1.2 2021/03/26 23:17:33 rillig Exp $	*/
# 3 "d_pr_22119.c"

/*
 * https://gnats.netbsd.org/22119
 *
 * Before 2021-02-28, lint crashed in cast() since the target type of the
 * cast is NULL.
*/

void
func1(void)
{
	void (*f1)(void);

	f1 = (void (*)(void))p;		/* expect: 'p' undefined [99] */
	f1 = (void *()(void))p;		/* crash before 2021-02-28 */
}
