/*	$NetBSD: msg_130.c,v 1.3 2021/01/31 11:12:07 rillig Exp $	*/
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
	return cond ? GREEN : MORNING;	/* expect: 130 */
}
