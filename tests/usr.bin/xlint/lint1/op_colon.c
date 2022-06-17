/*	$NetBSD: op_colon.c,v 1.3 2022/06/17 18:54:53 rillig Exp $	*/
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
	/* expect+1: ... 'pointer to const int' ... */
	sink(cond ? p : c);
	/* expect+1: ... 'pointer to volatile int' ... */
	sink(cond ? p : v);
	/* expect+1: ... 'pointer to const volatile int' ... */
	sink(cond ? p : cv);

	/* expect+1: ... 'pointer to const int' ... */
	sink(cond ? c : p);
	/* expect+1: ... 'pointer to const int' ... */
	sink(cond ? c : c);
	/* expect+1: ... 'pointer to const volatile int' ... */
	sink(cond ? c : v);
	/* expect+1: ... 'pointer to const volatile int' ... */
	sink(cond ? c : cv);

	/* expect+1: ... 'pointer to volatile int' ... */
	sink(cond ? v : p);
	/* expect+1: ... 'pointer to const volatile int' ... */
	sink(cond ? v : c);
	/* expect+1: ... 'pointer to volatile int' ... */
	sink(cond ? v : v);
	/* expect+1: ... 'pointer to const volatile int' ... */
	sink(cond ? v : cv);

	/* expect+1: ... 'pointer to const volatile int' ... */
	sink(cond ? cv : p);
	/* expect+1: ... 'pointer to const volatile int' ... */
	sink(cond ? cv : c);
	/* expect+1: ... 'pointer to const volatile int' ... */
	sink(cond ? cv : v);
	/* expect+1: ... 'pointer to const volatile int' ... */
	sink(cond ? cv : cv);
}
