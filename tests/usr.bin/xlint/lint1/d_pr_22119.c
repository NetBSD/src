/*	$NetBSD: d_pr_22119.c,v 1.4 2022/04/01 22:28:21 rillig Exp $	*/
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

	/* expect+1: error: 'p' undefined [99] */
	f1 = (void (*)(void))p;
	/* expect+2: error: function returns illegal type 'function(void) returning pointer to void' [15] */
	/* expect+1: error: invalid cast from 'int' to 'function() returning pointer to function(void) returning pointer to void' [147] */
	f1 = (void *()(void))p;		/* crash before 2021-02-28 */
}
