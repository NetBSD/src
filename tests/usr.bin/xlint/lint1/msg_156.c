/*	$NetBSD: msg_156.c,v 1.2 2021/01/08 21:25:03 rillig Exp $	*/
# 3 "msg_156.c"

// Test for message: enum type mismatch, arg #%d [156]

enum color {
	RED,
	GREEN,
	BLUE
};

enum size {
	SMALL,
	MEDIUM,
	LARGE
};

void
print_color(enum color);

void
example(void)
{
	print_color(MEDIUM);
}
