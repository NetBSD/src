/*	$NetBSD: msg_013.c,v 1.4 2022/06/11 12:24:00 rillig Exp $	*/
# 3 "msg_013.c"

// Test for message: incomplete enum type '%s' [13]

enum tag;

/* XXX: why '<unnamed>'? */
/* expect+1: warning: incomplete enum type '<unnamed>' [13] */
void function(enum tag);

enum tag {
	CONSTANT
};
