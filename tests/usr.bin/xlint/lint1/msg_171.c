/*	$NetBSD: msg_171.c,v 1.7 2021/12/21 23:12:21 rillig Exp $	*/
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
 *
 * Before init.c 1.111 from 2021-03-23, lint could not handle these nested
 * initializations (the outer one for the variable 'p', the inner one for the
 * compound literal) and wrongly complained about a type mismatch between
 * 'struct point' and 'pointer to struct point'.
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
	};

	/*
	 * A sizeof expression is another way to create nested
	 * initializations.
	 */
	struct point p2 = {
		(int)sizeof(struct point){
			(int)sizeof(struct point){
				(int)sizeof(struct point){
					(int)sizeof(struct point){
						0,
						0,
					},
					0,
				},
				0,
			},
			0,
		},
		0,
	};
}
