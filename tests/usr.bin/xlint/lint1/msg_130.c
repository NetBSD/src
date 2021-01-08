/*	$NetBSD: msg_130.c,v 1.2 2021/01/08 21:25:03 rillig Exp $	*/
# 3 "msg_130.c"

// Test for message: enum type mismatch, op %s [130]

enum color {
	RED, GREEN, BLUE
};

enum daytime {
	NIGHT, MORNING, NOON, EVENING
};

int
example(_Bool cond)
{
	return cond ? GREEN : MORNING;
}
