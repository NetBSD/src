/*	$NetBSD: msg_241.c,v 1.11 2023/07/09 11:01:27 rillig Exp $	*/
# 3 "msg_241.c"

// Test for message: dubious operation '%s' on enum [241]
//
// As of February 2021, the option -e is not enabled by default in
// share/mk/sys.mk, therefore this message is neither well-known nor
// well-tested.

/* lint1-extra-flags: -e -X 351 */

/*
 * Enums are a possible implementation of bit-sets.
 */
enum color {
	RED	= 1 << 0,
	GREEN	= 1 << 1,
	BLUE	= 1 << 2
};

extern void sink_bool(_Bool);
extern void sink_int(int);
extern void sink_color(enum color);

void
example(void)
{
	enum color c = RED;

	/* expect+1: warning: dubious operation '!' on enum [241] */
	sink_bool(!c);
	/* expect+1: warning: dubious operation '~' on enum [241] */
	sink_color(~c);
	/* expect+1: warning: dubious operation '++x' on enum [241] */
	++c;
	/* expect+1: warning: dubious operation '--x' on enum [241] */
	--c;
	/* expect+1: warning: dubious operation 'x++' on enum [241] */
	c++;
	/* expect+1: warning: dubious operation 'x--' on enum [241] */
	c--;
	/* expect+1: warning: dubious operation '+' on enum [241] */
	sink_color(+c);
	/* expect+1: warning: dubious operation '-' on enum [241] */
	sink_color(-c);
	/* expect+1: warning: dubious operation '*' on enum [241] */
	sink_color(c * c);
	/* expect+1: warning: dubious operation '/' on enum [241] */
	sink_color(c / c);
	/* expect+1: warning: dubious operation '%' on enum [241] */
	sink_color(c % c);
	/* expect+1: warning: dubious operation '+' on enum [241] */
	sink_color(c + c);
	/* expect+1: warning: dubious operation '-' on enum [241] */
	sink_color(c - c);
	/* expect+1: warning: dubious operation '<<' on enum [241] */
	sink_color(c << c);
	/* expect+1: warning: dubious operation '>>' on enum [241] */
	sink_color(c >> c);

	sink_bool(c < c);
	sink_bool(c <= c);
	sink_bool(c > c);
	sink_bool(c >= c);
	sink_bool(c == c);
	sink_bool(c != c);

	/* expect+1: warning: dubious operation '&' on enum [241] */
	sink_color(c & c);
	/* expect+1: warning: dubious operation '^' on enum [241] */
	sink_color(c ^ c);
	/* expect+1: warning: dubious operation '|' on enum [241] */
	sink_color(c | c);

	/* expect+1: warning: dubious operation '&&' on enum [241] */
	sink_bool(c && c);
	/* expect+1: warning: dubious operation '||' on enum [241] */
	sink_bool(c || c);
	sink_color(c ? c : BLUE);

	c = GREEN;
	/* expect+1: warning: dubious operation '*=' on enum [241] */
	c *= c;
	/* expect+1: warning: dubious operation '/=' on enum [241] */
	c /= c;
	/* expect+1: warning: dubious operation '%=' on enum [241] */
	c %= c;
	/* expect+1: warning: dubious operation '+=' on enum [241] */
	c += c;
	/* expect+1: warning: dubious operation '-=' on enum [241] */
	c -= c;
	/* expect+1: warning: dubious operation '<<=' on enum [241] */
	c <<= c;
	/* expect+1: warning: dubious operation '>>=' on enum [241] */
	c >>= c;
	/* expect+1: warning: dubious operation '&=' on enum [241] */
	c &= c;
	/* expect+1: warning: dubious operation '^=' on enum [241] */
	c ^= c;
	/* expect+1: warning: dubious operation '|=' on enum [241] */
	c |= c;

	/* The cast to unsigned is required by GCC at WARNS=6. */
	/* expect+1: warning: dubious operation '&=' on enum [241] */
	c &= ~(unsigned)GREEN;
}

void
cover_typeok_enum(enum color c, int i)
{
	/* expect+2: warning: dubious operation '*' on enum [241] */
	/* expect+1: warning: combination of 'enum color' and 'int', op '>' [242] */
	if (c * i > 5)
		return;
}

const char *
color_name(enum color c)
{
	static const char *name[] = {
	    [RED] = "red",
	    [GREEN] = "green",
	    [BLUE] = "blue",
	};

	if (c == RED)
		return *(c + name); /* unusual but allowed */
	if (c == GREEN)
		return c[name]; /* even more unusual */
	return name[c];
}
