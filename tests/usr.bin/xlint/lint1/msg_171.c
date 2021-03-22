/*	$NetBSD: msg_171.c,v 1.5 2021/03/22 16:51:24 rillig Exp $	*/
# 3 "msg_171.c"

// Test for message: cannot assign to '%s' from '%s' [171]

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
	/*
	 * FIXME: The type mismatch in the above line occurs because lint
	 * wrongly assumes that there is only ever a single initialization
	 * going on, which takes place in initstk.
	 *
	 * In the debug log, this is marked by the two calls to initstack_init
	 * that are happily intermixed with 'begin initialization' and 'end
	 * initialization'.  This was not planned for, and it worked well
	 * before C99, since compound literals are a new feature from C99.
	 *
	 * The proper fix, as for so many similar problems is to not use
	 * global variables for things that have a limited lifetime, but
	 * instead let the grammar determine the lifetime and scope of these
	 * objects, which makes them only accessible when they can actually be
	 * used.
	 */
}
