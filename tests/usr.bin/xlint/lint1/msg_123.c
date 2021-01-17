/*	$NetBSD: msg_123.c,v 1.2 2021/01/17 16:00:16 rillig Exp $	*/
# 3 "msg_123.c"

// Test for message: illegal combination of %s (%s) and %s (%s), op %s [123]

void ok(_Bool);
void bad(_Bool);

void
compare(_Bool b, int i, double d, const char *p)
{
	ok(b < b);
	ok(b < i);
	ok(b < d);
	bad(b < p);		/* expect: 123 */
	ok(i < b);
	ok(i < i);
	ok(i < d);
	bad(i < p);		/* expect: 123 */
	ok(d < b);
	ok(d < i);
	ok(d < d);
	bad(d < p);		/* expect: 107 */
	bad(p < b);		/* expect: 123 */
	bad(p < i);		/* expect: 123 */
	bad(p < d);		/* expect: 107 */
	ok(p < p);
}
