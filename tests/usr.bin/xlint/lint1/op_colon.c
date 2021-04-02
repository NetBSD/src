/*	$NetBSD: op_colon.c,v 1.1 2021/04/02 17:25:04 rillig Exp $	*/
# 3 "op_colon.c"

/*
 * Test handling of the operator ':', as part of the '?:'.
 */

/* lint1-extra-flags: -p */

void sink(void *);

void
test_merge_qualifiers(_Bool cond, int *p, const int *c, volatile int *v,
		      const volatile int *cv)
{
	sink(cond ? p : p);
	sink(cond ? p : c);	/* expect: 'pointer to const int' */
	sink(cond ? p : v);	/* expect: 'pointer to volatile int' */
	sink(cond ? p : cv);	/* expect: 'pointer to const volatile int' */

	sink(cond ? c : p);	/* expect: 'pointer to const int' */
	sink(cond ? c : c);	/* expect: 'pointer to const int' */
	sink(cond ? c : v);	/* expect: 'pointer to const volatile int' */
	sink(cond ? c : cv);	/* expect: 'pointer to const volatile int' */

	sink(cond ? v : p);	/* expect: 'pointer to volatile int' */
	sink(cond ? v : c);	/* expect: 'pointer to const volatile int' */
	sink(cond ? v : v);	/* expect: 'pointer to volatile int' */
	sink(cond ? v : cv);	/* expect: 'pointer to const volatile int' */

	sink(cond ? cv : p);	/* expect: 'pointer to const volatile int' */
	sink(cond ? cv : c);	/* expect: 'pointer to const volatile int' */
	sink(cond ? cv : v);	/* expect: 'pointer to const volatile int' */
	sink(cond ? cv : cv);	/* expect: 'pointer to const volatile int' */
}
