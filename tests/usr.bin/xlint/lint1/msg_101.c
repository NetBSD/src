/*	$NetBSD: msg_101.c,v 1.5 2021/06/30 14:02:11 rillig Exp $	*/
# 3 "msg_101.c"

// Test for message: type '%s' does not have member '%s' [101]

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
	/* FIXME: "type 'int'" is wrong. */
	/* expect+1: error: type 'int' does not have member 'z' [101] */
	sink(ptr->z);
	/* FIXME: "type 'int'" is wrong. */
	/* expect+1: error: type 'int' does not have member 'z' [101] */
	sink(pt.z);

	/* mixed up '.' and '->' */
	/* TODO: mention actual type in the diagnostic */
	/* expect+1: error: left operand of '.' must be struct/union object [103] */
	sink(ptr.x);
	/* TODO: put actual type in 'quotes' */
	/* expect+1: error: left operand of '->' must be pointer to struct/union not struct point [104] */
	sink(pt->x);

	/* accessing a nonexistent member via the wrong operator */
	/* FIXME: "type 'int'" is wrong. */
	/* expect+1: error: type 'int' does not have member 'z' [101] */
	sink(ptr.z);
	/* FIXME: "type 'int'" is wrong. */
	/* expect+1: error: type 'int' does not have member 'z' [101] */
	sink(pt->z);
}
