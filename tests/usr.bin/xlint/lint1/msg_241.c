/*	$NetBSD: msg_241.c,v 1.3 2021/02/27 14:54:55 rillig Exp $	*/
# 3 "msg_241.c"

// Test for message: dubious operation on enum, op %s [241]
//
// As of February 2021, the option -e is not enabled by default in
// share/mk/sys.mk, therefore this message is neither well-known nor
// well-tested.

/* lint1-extra-flags: -e */

/*
 * Enums are a possible implementation of bit-sets.
 */
enum color {
	RED	= 1 << 0,
	GREEN	= 1 << 1,
	BLUE	= 1 << 2
};

extern void sink(int);

void
example(void)
{
	enum color c = RED;

	sink(!c);			/* expect: 241 */
	sink(~c);			/* expect: 241, 278 */
	++c;				/* expect: 241 */
	--c;				/* expect: 241 */
	c++;				/* expect: 241 */
	c--;				/* expect: 241 */
	sink(+c);			/* expect: 241, 278 */
	sink(-c);			/* expect: 241, 278 */
	sink(c * c);			/* expect: 241, 278 */
	sink(c / c);			/* expect: 241, 278 */
	sink(c % c);			/* expect: 241, 278 */
	sink(c + c);			/* expect: 241, 278 */
	sink(c - c);			/* expect: 241, 278 */
	sink(c << c);			/* expect: 241, 278 */
	sink(c >> c);			/* expect: 241, 278 */

	sink(c < c);
	sink(c <= c);
	sink(c > c);
	sink(c >= c);
	sink(c == c);
	sink(c != c);

	sink(c & c);			/* expect: 241, 278 */
	sink(c ^ c);			/* expect: 241, 278 */
	sink(c | c);			/* expect: 241, 278 */

	sink(c && c);			/* expect: 241 */
	sink(c || c);			/* expect: 241 */
	sink(c ? c : BLUE);		/* expect: 278 */

	c = GREEN;
	c *= c;				/* expect: 241 */
	c /= c;				/* expect: 241 */
	c %= c;				/* expect: 241 */
	c += c;				/* expect: 241 */
	c -= c;				/* expect: 241 */
	c <<= c;			/* expect: 241 */
	c >>= c;			/* expect: 241 */
	c &= c;				/* expect: 241 */
	c ^= c;				/* expect: 241 */
	c |= c;				/* expect: 241 */

	/* The cast to unsigned is required by GCC at WARNS=6. */
	c &= ~(unsigned)GREEN;		/* expect: 241 */
}
