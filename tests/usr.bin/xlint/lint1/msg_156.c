/*	$NetBSD: msg_156.c,v 1.6 2022/06/16 16:58:36 rillig Exp $	*/
# 3 "msg_156.c"

// Test for message: enum type mismatch, arg #%d (%s != %s) [156]

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

	/* expect+1: warning: enum type mismatch, arg #1 (enum color != enum size) [156] */
	print_color(MEDIUM);
	/* expect+1: warning: enum type mismatch, arg #1 (enum color != enum size) [156] */
	print_color(s);
}
