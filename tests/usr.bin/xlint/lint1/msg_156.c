/*	$NetBSD: msg_156.c,v 1.7 2022/06/22 19:23:18 rillig Exp $	*/
# 3 "msg_156.c"

// Test for message: function expects '%s', passing '%s' for arg #%d [156]

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

	/* expect+1: warning: function expects 'enum color', passing 'enum size' for arg #1 [156] */
	print_color(MEDIUM);
	/* expect+1: warning: function expects 'enum color', passing 'enum size' for arg #1 [156] */
	print_color(s);
}
