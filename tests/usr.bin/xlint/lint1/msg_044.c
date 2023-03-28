/*	$NetBSD: msg_044.c,v 1.6 2023/03/28 14:44:34 rillig Exp $	*/
# 3 "msg_044.c"

// Test for message: declaration of '%s %s' introduces new type in ANSI C [44]

/* lint1-extra-flags: -X 351 */

/* expect+1: warning: struct 'tag' never defined [233] */
struct tag;

void declaration(struct tag *);

void definition(void) {
	/* expect+2: warning: declaration of 'struct tag' introduces new type in ANSI C [44] */
	/* expect+1: warning: struct 'tag' never defined [233] */
	struct tag;
}
