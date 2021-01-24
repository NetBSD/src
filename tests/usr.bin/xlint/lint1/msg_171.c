/*	$NetBSD: msg_171.c,v 1.2 2021/01/24 16:12:45 rillig Exp $	*/
# 3 "msg_171.c"

// Test for message: assignment type mismatch (%s != %s) [171]

struct s {
	int member;
};

/*ARGSUSED*/
void
example(int i, void *vp, struct s *s)
{
	i = *s;			/* expect: 171 */
	*s = i;			/* expect: 171 */

	vp = *s;		/* expect: 171 */
	*s = vp;		/* expect: 171 */
}
