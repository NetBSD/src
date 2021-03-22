/*	$NetBSD: msg_171.c,v 1.3 2021/03/22 15:05:00 rillig Exp $	*/
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

/*
 * C99 6.5.2.5 says that a compound literal evaluates to an unnamed object
 * with automatic storage duration, like any normal named object.  It is an
 * lvalue, which means that it is possible to take the address of the object.
 * Seen in external/mpl/bind/dist/lib/dns/rbtdb.c, update_rrsetstats.
 */
void
pointer_to_compound_literal(void)
{
	struct point {
		int x;
		int y;
	};
	struct point *p = &(struct point){
	    12, 5,
	};			/* expect: 171 *//*FIXME*/
}
