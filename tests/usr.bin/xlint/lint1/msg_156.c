/*	$NetBSD: msg_156.c,v 1.4 2021/02/27 13:43:36 rillig Exp $	*/
# 3 "msg_156.c"

// Test for message: enum type mismatch, arg #%d [156]

enum color {
	RED	= 1 << 0,
	GREEN	= 1 << 1,
	BLUE	= 1 << 2
};

enum size {
	SMALL,
	MEDIUM,
	LARGE
};

void print_color(enum color);

void
example(enum color c, enum size s)
{
	print_color(GREEN);
	print_color(c);

	print_color(MEDIUM);		/* expect: 156 */
	print_color(s);			/* expect: 156 */
}
