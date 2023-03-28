/*	$NetBSD: msg_101.c,v 1.10 2023/03/28 14:44:34 rillig Exp $	*/
# 3 "msg_101.c"

// Test for message: type '%s' does not have member '%s' [101]

/* lint1-extra-flags: -X 351 */

struct point {
	int x, y;
};

void sink(int);

void
test(const struct point *ptr, const struct point pt)
{
	/* accessing an existing member */
	sink(ptr->x);
	sink(pt.x);

	/* accessing a nonexistent member */
	/* expect+1: error: type 'pointer to const struct point' does not have member 'z' [101] */
	sink(ptr->z);
	/* expect+1: error: type 'const struct point' does not have member 'z' [101] */
	sink(pt.z);

	/* mixed up '.' and '->' */
	/* expect+1: error: left operand of '.' must be struct or union, not 'pointer to const struct point' [103] */
	sink(ptr.x);
	/* expect+1: error: left operand of '->' must be pointer to struct or union, not 'struct point' [104] */
	sink(pt->x);

	/* accessing a nonexistent member via the wrong operator */
	/* expect+1: error: type 'pointer to const struct point' does not have member 'z' [101] */
	sink(ptr.z);
	/* expect+1: error: type 'struct point' does not have member 'z' [101] */
	sink(pt->z);
}
