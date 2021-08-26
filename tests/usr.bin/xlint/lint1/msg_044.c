/*	$NetBSD: msg_044.c,v 1.3 2021/08/26 19:23:25 rillig Exp $	*/
# 3 "msg_044.c"

// Test for message: declaration introduces new type in ANSI C: %s %s [44]

/* expect+1: warning: struct tag never defined [233] */
struct tag;

void declaration(struct tag *);

void definition(void) {
	/* expect+2: warning: declaration introduces new type in ANSI C: struct tag [44] */
	/* expect+1: warning: struct tag never defined [233] */
	struct tag;
}
